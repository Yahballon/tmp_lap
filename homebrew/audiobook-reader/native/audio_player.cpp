// SPDX-License-Identifier: GPL-3.0-or-later
#include "audio_player.hpp"
#include <SDL.h>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
#include <libavutil/mathematics.h>
#include <libswresample/swresample.h>
}
#include <algorithm>
#include <cmath>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
namespace audio {
namespace {
int interrupted(void* p){return static_cast<std::atomic<bool>*>(p)->load()?1:0;}
std::string av_error(int rc){char text[128]{};av_strerror(rc,text,sizeof(text));return text;}
}
void Player::stop(){
 quit_=true;
 if(active_){pthread_join(thread_,nullptr);active_=false;}
 std::lock_guard<std::mutex> guard(mutex_);
 status_=Status{};path_.clear();seekMicroseconds_=-1;
}
bool Player::start(const std::string& path,double resume){
 stop();
 struct stat st{};
 if(path.empty()||stat(path.c_str(),&st)!=0||!S_ISREG(st.st_mode)||st.st_size<1024)return false;
 {std::lock_guard<std::mutex> guard(mutex_);path_=path;status_.opening=true;status_.message="Opening audiobook...";}
 seekMicroseconds_=(int64_t)(std::max(0.0,resume)*1000000);
 quit_=false;paused_=false;
 pthread_attr_t attrs;pthread_attr_init(&attrs);pthread_attr_setstacksize(&attrs,2*1024*1024);
 const int err=pthread_create(&thread_,&attrs,entry,this);
 pthread_attr_destroy(&attrs);
 if(err){std::lock_guard<std::mutex> guard(mutex_);
  status_.opening=false;status_.message="Could not launch audio decoder";return false;}
 active_=true;return true;
}
void Player::toggle_pause(){
 std::lock_guard<std::mutex> guard(mutex_);
 if(!status_.playing)return;
 const bool next=!paused_.load();paused_=next;status_.paused=next;
 status_.message=next?"Paused":"Playing";
}
bool Player::seek(double seconds){
 if(!std::isfinite(seconds))return false;
 std::lock_guard<std::mutex> guard(mutex_);
 if(!status_.playing)return false;
 const double upper=status_.duration>0?std::max(0.0,status_.duration-0.25):172800.0;
 seekMicroseconds_=(int64_t)(std::clamp(seconds,0.0,upper)*1000000);
 status_.seeking=true;
 return true;
}
bool Player::skip(double delta){
 Status s=snapshot();return seek(s.seconds+delta);
}
Status Player::snapshot()const{std::lock_guard<std::mutex> guard(mutex_);return status_;}
void Player::decode(){
 AVFormatContext* fmt=avformat_alloc_context();
 AVCodecContext* codec=nullptr;AVPacket* packet=av_packet_alloc();AVFrame* frame=av_frame_alloc();
 SwrContext* resampler=nullptr;SDL_AudioDeviceID device=0;
 int index=-1,rate=48000;std::string error;
 bool eof=false;bool subsystem=false;
 if(!fmt||!packet||!frame){error="Out of memory";goto cleanup;}
 fmt->interrupt_callback.callback=interrupted;fmt->interrupt_callback.opaque=&quit_;
 {
  const int rc=avformat_open_input(&fmt,path_.c_str(),nullptr,nullptr);
  if(rc<0){error="Cannot read M4B: "+av_error(rc);goto cleanup;}
 }
 if(avformat_find_stream_info(fmt,nullptr)<0){error="Invalid or unsupported M4B";goto cleanup;}
 index=av_find_best_stream(fmt,AVMEDIA_TYPE_AUDIO,-1,-1,nullptr,0);
 if(index<0){error="Audiobook does not contain an audio track";goto cleanup;}
 {
  const AVCodec* decoder=avcodec_find_decoder(fmt->streams[index]->codecpar->codec_id);
  if(!decoder){error="Audio codec not available in native FFmpeg";goto cleanup;}
  codec=avcodec_alloc_context3(decoder);
  if(!codec||avcodec_parameters_to_context(codec,fmt->streams[index]->codecpar)<0||
      avcodec_open2(codec,decoder,nullptr)<0){error="Audio decoder initialization failed";goto cleanup;}
 }
 if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0){error=std::string("PS5 audio unavailable: ")+SDL_GetError();goto cleanup;}
 subsystem=true;
 {
  SDL_AudioSpec wanted{};wanted.freq=rate;wanted.format=AUDIO_S16SYS;wanted.channels=2;wanted.samples=2048;
  device=SDL_OpenAudioDevice(nullptr,0,&wanted,nullptr,0);
  if(!device){error=std::string("Audio device failed: ")+SDL_GetError();goto cleanup;}
 }
 {
  AVChannelLayout stereo=AV_CHANNEL_LAYOUT_STEREO;
  if(swr_alloc_set_opts2(&resampler,&stereo,AV_SAMPLE_FMT_S16,rate,
       &codec->ch_layout,codec->sample_fmt,codec->sample_rate,0,nullptr)<0||
       !resampler||swr_init(resampler)<0){error="Audio sample conversion unavailable";goto cleanup;}
 }
 SDL_PauseAudioDevice(device,0);
 {
  double duration=0;
  if(fmt->duration>0&&fmt->duration!=AV_NOPTS_VALUE)duration=(double)fmt->duration/AV_TIME_BASE;
  else if(fmt->streams[index]->duration>0)duration=fmt->streams[index]->duration*av_q2d(fmt->streams[index]->time_base);
  std::lock_guard<std::mutex> guard(mutex_);
  status_.duration=duration;status_.playing=true;status_.opening=false;status_.message="Playing M4B";
 }
 {
  bool wasPaused=false;
  int64_t discardUntil=-1;
  while(!quit_){
   int64_t seek=seekMicroseconds_.exchange(-1);
   if(seek>=0){
    const int64_t upper=fmt->duration>0&&fmt->duration!=AV_NOPTS_VALUE?fmt->duration-250000:seek;
    seek=std::max<int64_t>(0,std::min(seek,std::max<int64_t>(0,upper)));
    const int64_t start=fmt->start_time==AV_NOPTS_VALUE?0:fmt->start_time;
    const int rc=av_seek_frame(fmt,-1,start+seek,AVSEEK_FLAG_BACKWARD);
    if(rc>=0){
      avcodec_flush_buffers(codec);
      swr_close(resampler);swr_init(resampler);
      SDL_ClearQueuedAudio(device);discardUntil=seek;
      eof=false;
      std::lock_guard<std::mutex> guard(mutex_);
      status_.seconds=(double)seek/1000000;status_.seeking=false;status_.message="Seek complete";
    }else{
      std::lock_guard<std::mutex> guard(mutex_);
      status_.seeking=false;status_.message="Seek unavailable: "+av_error(rc);
    }
   }
   if(paused_){if(!wasPaused)SDL_PauseAudioDevice(device,1);wasPaused=true;usleep(20000);continue;}
   if(wasPaused){SDL_PauseAudioDevice(device,0);wasPaused=false;}
   if(eof)break;
   const int rc=av_read_frame(fmt,packet);
   if(rc<0){
    eof=true;
    avcodec_send_packet(codec,nullptr);
   }else if(packet->stream_index==index)avcodec_send_packet(codec,packet);
   av_packet_unref(packet);
   while(!quit_ && avcodec_receive_frame(codec,frame)==0){
    int64_t pts=frame->best_effort_timestamp;
    double t=pts==AV_NOPTS_VALUE?0:pts*av_q2d(fmt->streams[index]->time_base);
    if(discardUntil>=0 && pts!=AV_NOPTS_VALUE && t*1000000+10000<(double)discardUntil){
      av_frame_unref(frame);continue;
    }
    discardUntil=-1;
    const int cap=(int)av_rescale_rnd(swr_get_delay(resampler,codec->sample_rate)+frame->nb_samples,
                  rate,codec->sample_rate,AV_ROUND_UP);
    if(cap>0&&cap<192000){
     std::vector<uint8_t> pcm((size_t)cap*4);uint8_t* output[]={pcm.data()};
     const int n=swr_convert(resampler,output,cap,(const uint8_t**)frame->extended_data,frame->nb_samples);
     if(n>0){
      while(SDL_GetQueuedAudioSize(device)>rate*4*2&&!quit_)usleep(10000);
      if(!quit_ && SDL_QueueAudio(device,pcm.data(),(uint32_t)n*4)!=0){
       error="Audio queue failed";quit_=true;
      }
      std::lock_guard<std::mutex> guard(mutex_);
      if(pts!=AV_NOPTS_VALUE)status_.seconds=std::max(0.0,t);
      else status_.seconds+=n/(double)rate;
     }
    }
    av_frame_unref(frame);
   }
  }
  if(!quit_&&error.empty())
   while(SDL_GetQueuedAudioSize(device)>0&&!quit_)usleep(20000);
 }
cleanup:
 if(device){SDL_ClearQueuedAudio(device);SDL_CloseAudioDevice(device);}
 if(subsystem)SDL_QuitSubSystem(SDL_INIT_AUDIO);
 swr_free(&resampler);av_frame_free(&frame);av_packet_free(&packet);
 avcodec_free_context(&codec);if(fmt)avformat_close_input(&fmt);
 std::lock_guard<std::mutex> guard(mutex_);
 status_.opening=false;status_.playing=false;status_.seeking=false;
 status_.ended=!quit_;status_.paused=false;
 if(!error.empty())status_.message=error;
 else if(!quit_)status_.message="Audiobook finished";
}
} // namespace audio
