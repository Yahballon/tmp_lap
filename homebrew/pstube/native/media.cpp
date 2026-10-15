// SPDX-License-Identifier: GPL-3.0-or-later
#include "media.hpp"
#include <SDL.h>
#include "ca_bundle.inc"
extern "C" {
#include <libavutil/dict.h>
#include <libavutil/error.h>
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
bool media_host(const std::string& u){
 if(u.rfind("https://",0)!=0||u.size()>16000)return false;
 const auto end=u.find_first_of("/?#",8);
 const std::string host=u.substr(8,end==std::string::npos?std::string::npos:end-8);
 return host=="googlevideo.com"||
   (host.size()>16&&host.compare(host.size()-16,16,".googlevideo.com")==0);
}
// FFmpeg's native TLS backend reads roots from this on-console file.
std::string root_certificate_file(){
 static const std::string path=[]{
  const char* names[]={"/data/pstube/certificates.pem","/download0/pstube-roots.pem"};
  mkdir("/data/pstube",0700);
  for(const char* target:names){
   FILE* f=fopen(target,"wb");if(!f)continue;
   const size_t size=strlen(kCaBundle);
   const bool ok=fwrite(kCaBundle,1,size,f)==size&&fflush(f)==0&&fsync(fileno(f))==0;
   if(fclose(f)==0&&ok)return std::string(target);
  }
  return std::string();
 }();
 return path;
}
int interrupt_cb(void* ctx){return ((std::atomic<bool>*)ctx)->load() ? 1:0;}
void sleep_us(int n){if(n>0)usleep((useconds_t)n);}
}
MediaPlayer::~MediaPlayer(){close();}
void MediaPlayer::close(){
 quit_.store(true);
 if(!joined_){pthread_join(worker_,nullptr);joined_=true;}
 std::lock_guard<std::mutex> g(lock_);
 pixels_.clear();state_=PlaybackState{};path_.clear();audioPath_.clear();remote_=false;sequence_=0;seekDeltaUs_=0;
}
bool MediaPlayer::open(const std::string& path){
 close();struct stat st{};
 if(path.empty()||stat(path.c_str(),&st)!=0||st.st_size<1024)return false;
 {std::lock_guard<std::mutex> g(lock_);path_=path;remote_=false;state_.opening=true;state_.message="Opening local video...";}
 quit_=false;paused_=false;
 pthread_attr_t attrs;pthread_attr_init(&attrs);pthread_attr_setstacksize(&attrs,2*1024*1024);
 int rc=pthread_create(&worker_,&attrs,thread_entry,this);
 pthread_attr_destroy(&attrs);
 if(rc){std::lock_guard<std::mutex> g(lock_);state_.opening=false;state_.message="Decoder worker failed";return false;}
 joined_=false;return true;
}
bool MediaPlayer::openStream(const std::string& url){
 close();
 if(!media_host(url))return false;
 const std::string ca=root_certificate_file();
 if(ca.empty()){std::lock_guard<std::mutex> g(lock_);state_.message="Trusted TLS certificates unavailable";return false;}
 {
  std::lock_guard<std::mutex> g(lock_);
  path_=url;remote_=true;state_.opening=true;state_.message="Buffering stream...";
 }
 quit_=false;paused_=false;
 pthread_attr_t attrs;pthread_attr_init(&attrs);pthread_attr_setstacksize(&attrs,2*1024*1024);
 int rc=pthread_create(&worker_,&attrs,thread_entry,this);
 pthread_attr_destroy(&attrs);
 if(rc){
  std::lock_guard<std::mutex> g(lock_);
  state_.opening=false;state_.message="Could not start streaming worker";
  return false;
 }
 joined_=false;return true;
}
bool MediaPlayer::openAdaptiveStream(const std::string& videoUrl,const std::string& audioUrl){
 close();
 if(!media_host(videoUrl)||!media_host(audioUrl))return false;
 if(root_certificate_file().empty())return false;
 {
  std::lock_guard<std::mutex> g(lock_);
  path_=videoUrl;audioPath_=audioUrl;remote_=true;
  state_.opening=true;state_.message="Opening HD video and audio...";
 }
 quit_=false;paused_=false;
 pthread_attr_t attrs;pthread_attr_init(&attrs);pthread_attr_setstacksize(&attrs,2*1024*1024);
 const int rc=pthread_create(&worker_,&attrs,thread_entry,this);
 pthread_attr_destroy(&attrs);
 if(rc){std::lock_guard<std::mutex> g(lock_);state_.opening=false;state_.message="HD worker failed";return false;}
 joined_=false;return true;
}
// Native integration test path: separate files emulate the YouTube adaptive pair.
bool MediaPlayer::openAdaptiveFiles(const std::string& videoPath,const std::string& audioPath){
 close();
 struct stat video{},audio{};
 if(videoPath.empty()||audioPath.empty()||stat(videoPath.c_str(),&video)!=0||
    stat(audioPath.c_str(),&audio)!=0||video.st_size<1024||audio.st_size<1024)return false;
 {
  std::lock_guard<std::mutex> g(lock_);path_=videoPath;audioPath_=audioPath;remote_=false;
  state_.opening=true;state_.message="Opening separate media tracks...";
 }
 quit_=false;paused_=false;
 pthread_attr_t attrs;pthread_attr_init(&attrs);pthread_attr_setstacksize(&attrs,2*1024*1024);
 const int rc=pthread_create(&worker_,&attrs,thread_entry,this);
 pthread_attr_destroy(&attrs);
 if(rc){std::lock_guard<std::mutex> g(lock_);state_.opening=false;return false;}
 joined_=false;return true;
}
void MediaPlayer::pause(){bool v=!paused_.load();paused_.store(v);std::lock_guard<std::mutex> g(lock_);state_.paused=v;}
bool MediaPlayer::seekRelative(int deltaSeconds){
 if(deltaSeconds==0||deltaSeconds>300||deltaSeconds< -300)return false;
 {
  std::lock_guard<std::mutex> g(lock_);
  if(!state_.playing||state_.ended||quit_.load())return false;
  state_.seeking=true;
  state_.message="Seeking "+std::to_string(deltaSeconds>0?deltaSeconds:-deltaSeconds)+
     " seconds "+(deltaSeconds>0?"forward":"backward");
 }
 seekDeltaUs_.fetch_add((int64_t)deltaSeconds*AV_TIME_BASE,std::memory_order_release);
 return true;
}
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
 AVFormatContext* afmt=nullptr;
 AVPacket* apkt=av_packet_alloc();AVFrame* aframe=av_frame_alloc();
 int separateAi=-1;int64_t audioNextUs=0;bool audioEof=false;
 AVFrame* frame=av_frame_alloc();AVPacket* pkt=av_packet_alloc();
 SwsContext* sws=nullptr;SwrContext* swr=nullptr;
 SDL_AudioDeviceID audio=0;
 std::string error;
 int vi=-1,ai=-1;
 {
  AVDictionary* opts=nullptr;
  if(remote_){
   const std::string cert=root_certificate_file();
   av_dict_set(&opts,"tls_verify","1",0);
   av_dict_set(&opts,"ca_file",cert.c_str(),0);
   av_dict_set(&opts,"rw_timeout","12000000",0);
   av_dict_set(&opts,"timeout","12000000",0);
   av_dict_set(&opts,"reconnect","1",0);
   av_dict_set(&opts,"reconnect_streamed","1",0);
   av_dict_set(&opts,"reconnect_delay_max","3",0);
   av_dict_set(&opts,"user_agent",audioPath_.empty()?
     "com.google.android.youtube/21.26.364 (Linux; U; Android 11) gzip":
     "com.google.android.apps.youtube.vr.oculus/1.65.10 (Linux; U; Android 12L) gzip",0);
   av_dict_set(&opts,"multiple_requests","1",0);
  }
  if(remote_ && avformat_network_init()<0){
   av_dict_free(&opts);error="FFmpeg network stack unavailable";goto cleanup;
  }
  const int opened=avformat_open_input(&fmt,path_.c_str(),nullptr,&opts);
  av_dict_free(&opts);
  if(opened<0){
   char reason[128]{};
   av_strerror(opened,reason,sizeof(reason));
   error=std::string(remote_?"Verified HTTPS stream failed: ":"Unable to open MP4: ")+reason;
   goto cleanup;
  }
 }
 if(avformat_find_stream_info(fmt,nullptr)<0){error=remote_?"Cannot probe HTTPS video/audio tracks":"Invalid media streams";goto cleanup;}
 if(!audioPath_.empty()){
  AVDictionary* opts=nullptr;
  if(remote_){
   const std::string cert=root_certificate_file();
   av_dict_set(&opts,"tls_verify","1",0);av_dict_set(&opts,"ca_file",cert.c_str(),0);
   av_dict_set(&opts,"rw_timeout","12000000",0);av_dict_set(&opts,"timeout","12000000",0);
   av_dict_set(&opts,"reconnect","1",0);av_dict_set(&opts,"reconnect_streamed","1",0);
   av_dict_set(&opts,"user_agent",audioPath_.empty()?
     "com.google.android.youtube/21.26.364 (Linux; U; Android 11) gzip":
     "com.google.android.apps.youtube.vr.oculus/1.65.10 (Linux; U; Android 12L) gzip",0);
  }
  const int openedAudio=avformat_open_input(&afmt,audioPath_.c_str(),nullptr,&opts);
  av_dict_free(&opts);
  if(openedAudio<0){error="Unable to open adaptive audio stream";goto cleanup;}
  if(avformat_find_stream_info(afmt,nullptr)<0){error="Unable to probe adaptive audio";goto cleanup;}
  separateAi=av_find_best_stream(afmt,AVMEDIA_TYPE_AUDIO,-1,-1,nullptr,0);
  if(separateAi<0){error="Adaptive audio track missing";goto cleanup;}
 }
 vi=av_find_best_stream(fmt,AVMEDIA_TYPE_VIDEO,-1,-1,nullptr,0);
 if(vi<0){error="Video track missing";goto cleanup;}
 {
  const AVCodec* codec=avcodec_find_decoder(fmt->streams[vi]->codecpar->codec_id);
  if(!codec){error="Video codec unavailable";goto cleanup;}
  vc=avcodec_alloc_context3(codec);
  if(!vc||avcodec_parameters_to_context(vc,fmt->streams[vi]->codecpar)<0||avcodec_open2(vc,codec,nullptr)<0){error="Video decoder failed";goto cleanup;}
 }
 ai=afmt?separateAi:av_find_best_stream(fmt,AVMEDIA_TYPE_AUDIO,-1,vi,nullptr,0);
 if(ai>=0){
  AVFormatContext* source=afmt?afmt:fmt;
  const AVCodec* codec=avcodec_find_decoder(source->streams[ai]->codecpar->codec_id);
  if(codec) {ac=avcodec_alloc_context3(codec);
   if(!ac||avcodec_parameters_to_context(ac,source->streams[ai]->codecpar)<0||avcodec_open2(ac,codec,nullptr)<0){avcodec_free_context(&ac);ai=-1;}
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
 {
  double duration=0;
  if(fmt->duration!=AV_NOPTS_VALUE&&fmt->duration>0)duration=(double)fmt->duration/AV_TIME_BASE;
  else if(fmt->streams[vi]->duration!=AV_NOPTS_VALUE&&fmt->streams[vi]->duration>0)
   duration=fmt->streams[vi]->duration*av_q2d(fmt->streams[vi]->time_base);
  std::lock_guard<std::mutex> g(lock_);
  state_.opening=false;state_.playing=true;state_.width=vc->width;state_.height=vc->height;
  state_.duration=duration;
  state_.message=afmt?"HD video + AAC audio":remote_?"Streaming over HTTPS":"Playing local MP4";
 }
 {
  const AVRational tb=fmt->streams[vi]->time_base;
  const AVRational rate=fmt->streams[vi]->avg_frame_rate;
  const double fps=rate.num>0&&rate.den>0?av_q2d(rate):30;
  const double default_step=1000000.0/std::clamp(fps,1.0,120.0);
  int64_t wall_origin=av_gettime_relative();
  int64_t first_pts=AV_NOPTS_VALUE;
  int64_t pause_at=0;
  int64_t discardBeforeUs=-1;
  while(!quit_.load()){
   // FFmpeg format/codec/audio contexts belong only to this worker thread.
   // Seek requests arrive from the DualSense input thread through an atomic.
   const int64_t delta=seekDeltaUs_.exchange(0,std::memory_order_acq_rel);
   if(delta){
    double current=0,duration=0;
    {std::lock_guard<std::mutex> g(lock_);current=state_.seconds;duration=state_.duration;}
    const double next=std::clamp(current+(double)delta/AV_TIME_BASE,0.0,
                                duration>0?std::max(0.0,duration-0.10):24.0*3600);
    const int64_t relativeUs=(int64_t)(next*AV_TIME_BASE);
    const int64_t offset=fmt->start_time!=AV_NOPTS_VALUE?fmt->start_time:0;
    const int64_t wanted=offset+relativeUs;
    // AVSEEK_FLAG_BACKWARD finds a decodeable keyframe; old frames are then
    // discarded until the desired timestamp is reached.
    const int rc=av_seek_frame(fmt,-1,wanted,AVSEEK_FLAG_BACKWARD);
    int arc=0;
    if(rc>=0&&afmt){
     const int64_t astart=afmt->start_time!=AV_NOPTS_VALUE?afmt->start_time:0;
     arc=av_seek_frame(afmt,-1,astart+relativeUs,AVSEEK_FLAG_BACKWARD);
    }
    if(rc>=0&&arc>=0){
     audioNextUs=relativeUs;audioEof=false;
     avcodec_flush_buffers(vc);if(ac)avcodec_flush_buffers(ac);
     if(swr){swr_close(swr);swr_init(swr);}
     if(audio)SDL_ClearQueuedAudio(audio);
     if(first_pts==AV_NOPTS_VALUE){
      const auto start=fmt->streams[vi]->start_time;
      first_pts=start!=AV_NOPTS_VALUE?start:0;
     }
     discardBeforeUs=wanted;
     wall_origin=av_gettime_relative()-relativeUs;
     if(paused_.load())pause_at=av_gettime_relative();
     {std::lock_guard<std::mutex> g(lock_);
      state_.seconds=next;state_.seeking=false;state_.message="Seek complete";
      pixels_.clear();++sequence_;
     }
    }else{
     char reason[128]{};av_strerror(rc,reason,sizeof(reason));
     std::lock_guard<std::mutex> g(lock_);
     state_.seeking=false;state_.message=std::string("Seeking unavailable: ")+reason;
    }
   }
   if(paused_.load()){
    if(!pause_at){pause_at=av_gettime_relative();if(audio)SDL_PauseAudioDevice(audio,1);}
    sleep_us(20000);continue;
   }
   if(pause_at){wall_origin+=av_gettime_relative()-pause_at;pause_at=0;if(audio)SDL_PauseAudioDevice(audio,0);}
   if(av_read_frame(fmt,pkt)<0)break;
   if(quit_)break;
   AVCodecContext* c=pkt->stream_index==vi?vc:pkt->stream_index==ai?ac:nullptr;
   if(c&&avcodec_send_packet(c,pkt)>=0){
    while(!quit_.load()&&avcodec_receive_frame(c,frame)==0){
     if(discardBeforeUs>=0&&frame->best_effort_timestamp!=AV_NOPTS_VALUE){
      const int64_t frameUs=av_rescale_q(frame->best_effort_timestamp,
         fmt->streams[pkt->stream_index]->time_base,AV_TIME_BASE_Q);
      if(frameUs+25000<discardBeforeUs){av_frame_unref(frame);continue;}
     }
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
      // The HD video and audio are independent HTTP inputs. Feed the AAC
      // decoder ahead of the video clock rather than dropping sound.
      if(afmt&&ac&&!audioEof){
       const int64_t targetUs=frame->best_effort_timestamp==AV_NOPTS_VALUE?
        (int64_t)((double)(av_gettime_relative()-wall_origin)):
        av_rescale_q(frame->best_effort_timestamp,fmt->streams[vi]->time_base,AV_TIME_BASE_Q);
       for(int work=0;work<14&&!quit_&&!audioEof&&audioNextUs<targetUs+300000;work++){
        if(av_read_frame(afmt,apkt)<0){audioEof=true;break;}
        if(apkt->stream_index!=separateAi){av_packet_unref(apkt);continue;}
        int64_t pts=apkt->pts!=AV_NOPTS_VALUE?apkt->pts:apkt->dts;
        if(pts!=AV_NOPTS_VALUE)audioNextUs=av_rescale_q(pts,afmt->streams[separateAi]->time_base,AV_TIME_BASE_Q);
        if(avcodec_send_packet(ac,apkt)>=0){
         while(!quit_&&avcodec_receive_frame(ac,aframe)==0){
          if(audio&&swr&&audioNextUs+25000>=discardBeforeUs){
           const int cap=(int)av_rescale_rnd(swr_get_delay(swr,ac->sample_rate)+aframe->nb_samples,48000,ac->sample_rate,AV_ROUND_UP);
           if(cap>0&&cap<96000){
            std::vector<uint8_t> pcm((size_t)cap*4);uint8_t* dst[]={pcm.data()};
            const int n=swr_convert(swr,dst,cap,(const uint8_t**)aframe->extended_data,aframe->nb_samples);
            if(n>0){
             while(SDL_GetQueuedAudioSize(audio)>48000*4&&!quit_)sleep_us(10000);
             if(!quit_)SDL_QueueAudio(audio,pcm.data(),(uint32_t)n*4);
            }
           }
          }
          av_frame_unref(aframe);
         }
        }
        av_packet_unref(apkt);
        audioNextUs+=(int64_t)AV_TIME_BASE*1024/48000;
       }
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
 av_packet_free(&apkt);av_frame_free(&aframe);
 avcodec_free_context(&vc);avcodec_free_context(&ac);
 if(fmt)avformat_close_input(&fmt);
 if(afmt)avformat_close_input(&afmt);
 std::lock_guard<std::mutex> g(lock_);
 state_.playing=false;state_.opening=false;state_.ended=!quit_.load();
 if(!error.empty())state_.message=error;
 else if(!quit_)state_.message="Playback finished";
}
}
