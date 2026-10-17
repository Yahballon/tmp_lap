// SPDX-License-Identifier: GPL-3.0-or-later
// eSpeak NG 1.52.0 synthesis + PS5 SDL2 queued PCM playback, no network.
#include "tts_reader.hpp"
#include "espeak-ng/speak_lib.h"
#include <SDL.h>
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>
namespace bible_tts {
namespace {
Speaker* speaker=nullptr; // Only one instance/worker active in this app.
constexpr int outputRate=48000, channels=2;
int receive(short* samples,int frames,espeak_EVENT* events){
 int begins[1024]{},sizes[1024]{},times[1024]{},count=0;
 if(events)for(int i=0;i<2048&&events[i].type!=espeakEVENT_LIST_TERMINATED;++i){
  if(events[i].type==espeakEVENT_WORD&&events[i].length>0&&count<1024){
   begins[count]=events[i].text_position;
   sizes[count]=events[i].length;times[count]=events[i].audio_position;++count;
  }
 }
 return speaker?speaker->collect(samples,frames,count,begins,sizes,times):1;
}
}
size_t utf8Offset(const std::string& value,size_t characters){
 size_t i=0;
 while(characters--&&i<value.size()){
  ++i;
  while(i<value.size()&&((unsigned char)value[i]&0xc0)==0x80)++i;
 }
 return i;
}
bool nextVerse(const bible::Library& lib,bible::Ref& ref){
 const auto& verses=lib.chapter(ref.book,ref.chapter);
 for(size_t i=0;i<verses.size();++i)if(verses[i].number==ref.verse){
  if(i+1<verses.size()){ref.verse=verses[i+1].number;return true;}
  break;
 }
 if(ref.chapter<lib.chapterCount(ref.book)){
  ++ref.chapter;
 }else if(ref.book<65){++ref.book;ref.chapter=1;}
 else return false;
 const auto& next=lib.chapter(ref.book,ref.chapter);
 if(next.empty())return false;
 ref.verse=next.front().number;return true;
}
void Speaker::status(Snapshot s){std::lock_guard<std::mutex> lock(mutex_);status_=std::move(s);}
Snapshot Speaker::snapshot() const{std::lock_guard<std::mutex> lock(mutex_);return status_;}
bool Speaker::active() const{return snapshot().active;}
bool Speaker::start(const bible::Library& lib,bible::Ref from,int lang){
 stop();
 if(!lib.verse(from))return false;
 library_=&lib;from_=from;language_=std::clamp(lang,0,3);cancelled_=false;
 Snapshot st{};st.active=true;st.ref=from;status(st);
 pthread_attr_t attrs{};pthread_attr_init(&attrs);pthread_attr_setstacksize(&attrs,2*1024*1024);
 int err=pthread_create(&thread_,&attrs,entry,this);pthread_attr_destroy(&attrs);
 if(err){st.active=false;st.error="TTS thread unavailable";status(st);return false;}
 joined_=false;return true;
}
void Speaker::stop(){
 cancelled_=true;
 if(!joined_){pthread_join(thread_,nullptr);joined_=true;}
 Snapshot st=snapshot();st.active=false;st.playing=false;st.wordBegin=st.wordEnd=0;
 st.error.clear();status(st);
}
int Speaker::collect(const short* samples,int frames,int count,const int* begin,const int* length,const int* ms){
 if(cancelled_)return 1;
 if(samples&&frames>0&&frames<200000){
  pcm_.insert(pcm_.end(),samples,samples+frames);
 }
 for(int i=0;i<count;++i){
  // eSpeak's text_position is 1-based. Convert UTF-8 codepoints to byte spans.
  size_t a=utf8Offset(phrase_,(size_t)std::max(0,begin[i]-1));
  size_t z=utf8Offset(phrase_,(size_t)std::max(0,begin[i]+length[i]-1));
  if(a<z&&z<=phrase_.size()&&(marks_.empty()||marks_.back().begin!=a))
   marks_.push_back({std::max(0,ms[i]),a,z});
 }
 return cancelled_?1:0;
}
void Speaker::run(){
 SDL_AudioDeviceID device=0;bool subsystem=false,initialized=false;
 std::string error;
 int rate=espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS,100,"/app0/assets/tts",0);
 if(rate<0)rate=espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS,100,"/data/homebrew/PPSA99793/assets/tts",0);
 if(rate<0){error="Offline speech dictionaries not found";goto done;}
 initialized=true;
 {
  const char* voices[]={"en","de","fr","es"};
  if(espeak_SetVoiceByName(voices[language_])!=EE_OK){error="Voice unavailable";goto done;}
 }
 speaker=this;espeak_SetSynthCallback(receive);
 if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0){error=SDL_GetError();goto done;}
 subsystem=true;
 {
  SDL_AudioSpec wanted{};wanted.freq=outputRate;wanted.format=AUDIO_S16SYS;
  wanted.channels=channels;wanted.samples=2048;
  device=SDL_OpenAudioDevice(nullptr,0,&wanted,nullptr,0);
  if(!device){error=SDL_GetError();goto done;}
 }
 SDL_PauseAudioDevice(device,0);
 {
  bible::Ref ref=from_;
  while(!cancelled_){
   const bible::Verse* verse=library_->verse(ref);if(!verse)break;
   phrase_=verse->text;pcm_.clear();marks_.clear();
   Snapshot st{};st.active=true;st.ref=ref;status(st);
   auto result=espeak_Synth(phrase_.c_str(),phrase_.size()+1,0,POS_CHARACTER,0,
                            espeakCHARS_UTF8,nullptr,nullptr);
   if(cancelled_)break;
   if(result!=EE_OK||pcm_.empty()){error="Offline synthesis failed";break;}
   SDL_AudioCVT cvt{};
   if(SDL_BuildAudioCVT(&cvt,AUDIO_S16SYS,1,rate,AUDIO_S16SYS,channels,outputRate)<0){
    error=SDL_GetError();break;
   }
   cvt.len=(int)(pcm_.size()*sizeof(short));
   if(cvt.len>10*1024*1024){error="Verse audio exceeds limit";break;}
   std::vector<unsigned char> audio((size_t)cvt.len*std::max(1,cvt.len_mult));
   std::memcpy(audio.data(),pcm_.data(),(size_t)cvt.len);
   cvt.buf=audio.data();
   if(cvt.needed&&SDL_ConvertAudio(&cvt)!=0){error=SDL_GetError();break;}
   if(SDL_QueueAudio(device,cvt.buf,(Uint32)cvt.len_cvt)!=0){error=SDL_GetError();break;}
   const auto total=(Uint32)cvt.len_cvt;
   st.playing=true;status(st);
   int old=-1;
   while(!cancelled_){
    const auto remain=SDL_GetQueuedAudioSize(device);
    const double played=((double)(total-std::min(total,remain))*1000.0)/
                        (outputRate*channels*sizeof(short));
    int chosen=-1;
    for(size_t i=0;i<marks_.size();++i){
     if(marks_[i].timeMs<=played+10.0)chosen=(int)i;else break;
    }
    if(chosen!=old){
     old=chosen;st.wordBegin=chosen>=0?marks_[(size_t)chosen].begin:0;
     st.wordEnd=chosen>=0?marks_[(size_t)chosen].end:0;status(st);
    }
    if(remain==0)break;
    usleep(10000);
   }
   if(cancelled_||!nextVerse(*library_,ref))break;
  }
 }
done:
 if(device){SDL_ClearQueuedAudio(device);SDL_CloseAudioDevice(device);}
 if(subsystem)SDL_QuitSubSystem(SDL_INIT_AUDIO);
 speaker=nullptr;
 if(initialized)espeak_Terminate();
 Snapshot st=snapshot();st.active=false;st.playing=false;
 st.wordBegin=st.wordEnd=0;st.error=error;status(st);
}
}
