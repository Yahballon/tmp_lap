// SPDX-License-Identifier: GPL-3.0-or-later
#include "media.hpp"
#include <SDL.h>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
namespace pstube {
namespace {
int interrupt_cb(void* ctx){return ((std::atomic<bool>*)ctx)->load() ? 1:0;}
void sleep_us(int n){if(n>0)usleep((useconds_t)n);}
}
MediaPlayer::~MediaPlayer(){close();}
void MediaPlayer::close(){
 quit_.store(true);
 if(!joined_){pthread_join(worker_,nullptr);joined_=true;}
 std::lock_guard<std::mutex> g(lock_);
 pixels_.clear();state_=PlaybackState{};path_.clear();sequence_=0;
}
bool MediaPlayer::open(const std::string& path){
 close();struct stat st{};
 if(path.empty()||stat(path.c_str(),&st)!=0||st.st_size<1024)return false;
 {std::lock_guard<std::mutex> g(lock_);path_=path;state_.opening=true;state_.message="Opening video...";}
 quit_=false;paused_=false;
 pthread_attr_t attrs;pthread_attr_init(&attrs);pthread_attr_setstacksize(&attrs,2*1024*1024);
 int rc=pthread_create(&worker_,&attrs,thread_entry,this);
 pthread_attr_destroy(&attrs);
 if(rc){std::lock_guard<std::mutex> g(lock_);state_.opening=false;state_.message="Decoder worker failed";return false;}
 joined_=false;return true;
}
void MediaPlayer::pause(){bool v=!paused_.load();paused_.store(v);std::lock_guard<std::mutex> g(lock_);state_.paused=v;}
PlaybackState MediaPlayer::snapshot()const{std::lock_guard<std::mutex> g(lock_);return state_;}
bool MediaPlayer::pull(std::vector<unsigned char>& pixels,int& w,int& h,uint64_t& seq){
 std::lock_guard<std::mutex> g(lock_);
 if(pixels_.empty()||seq==sequence_)return false;
 pixels=pixels_;w=state_.width;h=state_.height;seq=sequence_;return true;
}
void* MediaPlayer::thread_entry(void* obj){((MediaPlayer*)obj)->decode();return nullptr;}
void MediaPlayer::decode(){
 AVFormatContext* fmt=avformat_alloc_context();
 fmt->interrupt_callback.callback=interrupt_cb;fmt->interrupt_callback.opaque=&quit_;
 AVCodecContext* vc=nullptr; AVCodecContext* ac=nullptr;
 AVFrame* frame=av_frame_alloc();AVPacket* pkt=av_packet_alloc();
 SwsContext* sws=nullptr;SwrContext* swr=nullptr;
 SDL_AudioDeviceID audio=0;
 std::string error;
 int vi=-1,ai=-1;
 if(avformat_open_input(&fmt,path_.c_str(),nullptr,nullptr)<0){error="Unable to open MP4";goto cleanup;}
 if(avformat_find_stream_info(fmt,nullptr)<0){error="Invalid media streams";goto cleanup;}
 vi=av_find_best_stream(fmt,AVMEDIA_TYPE_VIDEO,-1,-1,nullptr,0);
 if(vi<0){error="Video track missing";goto cleanup;}
 {
  const AVCodec* codec=avcodec_find_decoder(fmt->streams[vi]->codecpar->codec_id);
  if(!codec){error="Video codec unavailable";goto cleanup;}
  vc=avcodec_alloc_context3(codec);
  if(!vc||avcodec_parameters_to_context(vc,fmt->streams[vi]->codecpar)<0||avcodec_open2(vc,codec,nullptr)<0){error="Video decoder failed";goto cleanup;}
 }
 ai=av_find_best_stream(fmt,AVMEDIA_TYPE_AUDIO,-1,vi,nullptr,0);
 if(ai>=0){
  const AVCodec* codec=avcodec_find_decoder(fmt->streams[ai]->codecpar->codec_id);
  if(codec) {ac=avcodec_alloc_context3(codec);
   if(!ac||avcodec_parameters_to_context(ac,fmt->streams[ai]->codecpar)<0||avcodec_open2(ac,codec,nullptr)<0){avcodec_free_context(&ac);ai=-1;}
  }
 }
 if(ac && SDL_InitSubSystem(SDL_INIT_AUDIO)==0){
  SDL_AudioSpec want{};want.freq=48000;want.format=AUDIO_S16SYS;want.channels=2;want.samples=2048;
  audio=SDL_OpenAudioDevice(nullptr,0,&want,nullptr,0);
  if(audio){SDL_PauseAudioDevice(audio,0);
   AVChannelLayout stereo=AV_CHANNEL_LAYOUT_STEREO;
   if(swr_alloc_set_opts2(&swr,&stereo,AV_SAMPLE_FMT_S16,48000,&ac->ch_layout,ac->sample_fmt,ac->sample_rate,0,nullptr)<0 || !swr || swr_init(swr)<0){swr_free(&swr);}
  }
 }
 if(!vc->width||!vc->height||vc->width>3840||vc->height>2160){error="Video resolution unsupported";goto cleanup;}
 sws=sws_getContext(vc->width,vc->height,vc->pix_fmt,vc->width,vc->height,AV_PIX_FMT_RGBA,SWS_BILINEAR,nullptr,nullptr,nullptr);
 if(!sws){error="Video pixel conversion unavailable";goto cleanup;}
 {std::lock_guard<std::mutex> g(lock_);state_.opening=false;state_.playing=true;state_.width=vc->width;state_.height=vc->height;state_.message="Playing local MP4";}
 {
  const AVRational tb=fmt->streams[vi]->time_base;
  const AVRational rate=fmt->streams[vi]->avg_frame_rate;
  const double fps=rate.num>0&&rate.den>0?av_q2d(rate):30;
  const double default_step=1000000.0/std::clamp(fps,1.0,120.0);
  int64_t wall_origin=av_gettime_relative();
  int64_t first_pts=AV_NOPTS_VALUE;
  int64_t pause_at=0;
  while(!quit_.load()&&av_read_frame(fmt,pkt)>=0){
   while(paused_.load()&&!quit_.load()){
    if(!pause_at)pause_at=av_gettime_relative();
    sleep_us(20000);
   }
   if(pause_at){wall_origin+=av_gettime_relative()-pause_at;pause_at=0;}
   if(quit_)break;
   AVCodecContext* c=pkt->stream_index==vi?vc:pkt->stream_index==ai?ac:nullptr;
   if(c&&avcodec_send_packet(c,pkt)>=0){
    while(!quit_.load()&&avcodec_receive_frame(c,frame)==0){
     if(c==vc){
      int w=vc->width,h=vc->height;
      std::vector<unsigned char> rgba((size_t)w*(size_t)h*4);
      uint8_t* dest[]={rgba.data(),nullptr,nullptr,nullptr};
      int lines[]={w*4,0,0,0};
      if(sws_scale(sws,frame->data,frame->linesize,0,h,dest,lines)>0){
       double elapsed=(double)(av_gettime_relative()-wall_origin);
       if(frame->best_effort_timestamp!=AV_NOPTS_VALUE){
        if(first_pts==AV_NOPTS_VALUE)first_pts=frame->best_effort_timestamp;
        double target=(frame->best_effort_timestamp-first_pts)*av_q2d(tb)*1000000;
        if(target>elapsed && target-elapsed<1000000) sleep_us((int)(target-elapsed));
       }else sleep_us((int)default_step);
       std::lock_guard<std::mutex> g(lock_);
       pixels_=std::move(rgba);state_.seconds=first_pts!=AV_NOPTS_VALUE?
        (frame->best_effort_timestamp-first_pts)*av_q2d(tb):elapsed/1000000.;++sequence_;
      }
     }else if(audio&&swr){
      const int cap=(int)av_rescale_rnd(swr_get_delay(swr,ac->sample_rate)+frame->nb_samples,48000,ac->sample_rate,AV_ROUND_UP);
      if(cap>0&&cap<48000*2){
       std::vector<uint8_t> pcm((size_t)cap*4);
       uint8_t* dst[]={pcm.data()};
       int n=swr_convert(swr,dst,cap,(const uint8_t**)frame->extended_data,frame->nb_samples);
       if(n>0){
        while(SDL_GetQueuedAudioSize(audio)>48000*4&&!quit_)sleep_us(10000);
        if(!quit_)SDL_QueueAudio(audio,pcm.data(),(uint32_t)n*4);
       }
      }
     }
     av_frame_unref(frame);
    }
   }
   av_packet_unref(pkt);
  }
  if(audio&&!quit_)while(SDL_GetQueuedAudioSize(audio)>0&&!quit_)sleep_us(25000);
 }
cleanup:
 if(audio){SDL_ClearQueuedAudio(audio);SDL_CloseAudioDevice(audio);SDL_QuitSubSystem(SDL_INIT_AUDIO);}
 if(swr)swr_free(&swr);if(sws)sws_freeContext(sws);
 av_packet_free(&pkt);av_frame_free(&frame);
 avcodec_free_context(&vc);avcodec_free_context(&ac);
 if(fmt)avformat_close_input(&fmt);
 std::lock_guard<std::mutex> g(lock_);
 state_.playing=false;state_.opening=false;state_.ended=!quit_.load();
 if(!error.empty())state_.message=error;
 else if(!quit_)state_.message="Playback finished";
}
}
