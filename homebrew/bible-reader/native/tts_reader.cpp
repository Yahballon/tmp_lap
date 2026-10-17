// SPDX-License-Identifier: GPL-3.0-or-later
// eSpeak NG 1.52.0 synthesis + PS5 SDL2 queued PCM playback, no network.
#include "tts_reader.hpp"
#include "tts_log.hpp"
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
 if(!lib.verse(from)){bible_log::write("TTS_BAD_REF","book=%d chapter=%d verse=%d",from.book,from.chapter,from.verse);return false;}
 bible_log::write("TTS_START","language=%d book=%d chapter=%d verse=%d",lang,from.book,from.chapter,from.verse);
 library_=&lib;from_=from;language_=std::clamp(lang,0,3);cancelled_=false;
 Snapshot st{};st.active=true;st.ref=from;status(st);
 pthread_attr_t attrs{};pthread_attr_init(&attrs);pthread_attr_setstacksize(&attrs,2*1024*1024);
 int err=pthread_create(&thread_,&attrs,entry,this);pthread_attr_destroy(&attrs);
 if(err){st.active=false;st.error="TTS thread unavailable";status(st);bible_log::write("TTS_THREAD_ERR","pthread_create result=%d",err);return false;}
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
 const char* roots[]={"/app0/assets/tts","/data/homebrew/PPSA99793/assets/tts"};
 const char* chosenRoot=nullptr;
 int rate=-1;
 const char* languages[]={"en","de","fr","es"};
 const char* profiles[]={"gmw/en","gmw/de","roa/fr","roa/es"};
 for(const char* root:roots){
  const std::string data=std::string(root)+"/espeak-ng-data/";
  const long phon=bible_log::fileSize(data+"phondata");
  const long dict=bible_log::fileSize(data+languages[language_]+std::string("_dict"));
  const long profile=bible_log::fileSize(data+"lang/"+profiles[language_]);
  bible_log::write("TTS_PATH","root=%s phondata=%ld dictionary=%ld profile=%ld",root,phon,dict,profile);
  if(phon>0&&dict>0&&profile>0){chosenRoot=root;break;}
 }
 if(!chosenRoot){error="Offline voice data missing";bible_log::write("TTS_DATA_ERR","No complete language assets for %s",languages[language_]);goto done;}
 rate=espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS,100,chosenRoot,espeakINITIALIZE_DONT_EXIT);
 bible_log::write("TTS_INIT","sample_rate=%d root=%s",rate,chosenRoot);
 if(rate<=0){error="Offline voice initialization failed";goto done;}
 initialized=true;
 {
  const char* codes[]={"en","de","fr","es"};
  const char* profiles[]={"gmw/en","gmw/de","roa/fr","roa/es"};
  const char* code=codes[language_];
  const char* profile=profiles[language_];
  const espeak_VOICE** list=espeak_ListVoices(nullptr);
  int count=0;
  if(list)for(int i=0;i<256&&list[i];++i){
   ++count;
   bible_log::write("TTS_VOICE","id=%s name=%s",list[i]->identifier?list[i]->identifier:"?",
                    list[i]->name?list[i]->name:"?");
  }
  bible_log::write("TTS_VOICES","found=%d code=%s",count,code);
  espeak_VOICE filter{};
  filter.languages=const_cast<char*>(code);
  espeak_ERROR result=espeak_SetVoiceByProperties(&filter);
  bible_log::write("TTS_SELECT","locale=%s result=%d",code,(int)result);
  if(result!=EE_OK){
   result=espeak_SetVoiceByName(profile);
   bible_log::write("TTS_FALLBACK","profile=%s result=%d",profile,(int)result);
  }
  if(result!=EE_OK){error="Voice unavailable (see BibleReader log)";goto done;}
  const espeak_VOICE* chosen=espeak_GetCurrentVoice();
  bible_log::write("TTS_READY","voice=%s",chosen&&chosen->identifier?chosen->identifier:"?");
 }
 speaker=this;espeak_SetSynthCallback(receive);
 if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0){error=SDL_GetError();bible_log::write("TTS_SDL_ERR","SDL_InitSubSystem: %s",error.c_str());goto done;}
 bible_log::write("TTS_AUDIO","SDL driver=%s",SDL_GetCurrentAudioDriver()?SDL_GetCurrentAudioDriver():"?");
 subsystem=true;
 {
  SDL_AudioSpec wanted{};wanted.freq=outputRate;wanted.format=AUDIO_S16SYS;
  wanted.channels=channels;wanted.samples=2048;
  device=SDL_OpenAudioDevice(nullptr,0,&wanted,nullptr,0);
  if(!device){error=SDL_GetError();bible_log::write("TTS_SDL_ERR","OpenAudioDevice: %s",error.c_str());goto done;}
  bible_log::write("TTS_AUDIO","device opened id=%u",(unsigned)device);
 }
 SDL_PauseAudioDevice(device,0);
 {
  bible::Ref ref=from_;
  while(!cancelled_){
   const bible::Verse* verse=library_->verse(ref);if(!verse)break;
   phrase_=verse->text;pcm_.clear();marks_.clear();
   if(verse->number==1||ref.book==from_.book&&ref.chapter==from_.chapter&&ref.verse==from_.verse)
    bible_log::write("TTS_VERSE","book=%d chapter=%d verse=%d",ref.book,ref.chapter,ref.verse);
   Snapshot st{};st.active=true;st.ref=ref;status(st);
   auto result=espeak_Synth(phrase_.c_str(),phrase_.size()+1,0,POS_CHARACTER,0,
                            espeakCHARS_UTF8,nullptr,nullptr);
   if(cancelled_)break;
   if(result!=EE_OK||pcm_.empty()){error="Offline synthesis failed";
    bible_log::write("TTS_SYNTH_ERR","rc=%d pcm_samples=%zu book=%d chapter=%d verse=%d",(int)result,pcm_.size(),ref.book,ref.chapter,ref.verse);
    break;}
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
 bible_log::write(error.empty()?"TTS_FINISH":"TTS_ERROR","%s",error.empty()?"stopped or completed":error.c_str());
 Snapshot st=snapshot();st.active=false;st.playing=false;
 st.wordBegin=st.wordEnd=0;st.error=error;status(st);
}
}
