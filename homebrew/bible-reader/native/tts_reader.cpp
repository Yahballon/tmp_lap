// SPDX-License-Identifier: GPL-3.0-or-later
// eSpeak NG 1.52.0 synthesis + PS5 SDL2 queued PCM playback, no network.
#include "tts_reader.hpp"
#include "tts_log.hpp"
#include "espeak-ng/speak_lib.h"
#if defined(BIBLE_NATIVE_PIPER)
#include "piper_onnx.hpp"
#include "phoneme_ids.hpp"
#include "thorsten_phonemes.generated.hpp"
#include "voice_en_phonemes.generated.hpp"
#include "voice_fr_phonemes.generated.hpp"
#include "voice_es_phonemes.generated.hpp"
#include "voice_ru_phonemes.generated.hpp"
#include <chrono>
#endif
#include <SDL.h>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <string>
#include <vector>
#include <unistd.h>
namespace bible_tts {
namespace {
Speaker* speaker=nullptr; // Only one instance/worker active in this app.
constexpr int outputRate=48000, channels=2;
#if defined(BIBLE_NATIVE_PIPER)
std::string ipaForPiper(const std::string& utf8){
 // Use exactly the eSpeak voice already selected for this worker (German).
 // API advances its input pointer sentence by sentence.
 const void* ptr=utf8.c_str();
 std::string result;
 for(int segment=0;ptr&&segment<256;++segment){
  const void* before=ptr;
  const char* chunk=espeak_TextToPhonemes(&ptr,espeakCHARS_UTF8,espeakPHONEMES_IPA);
  if(!chunk)break;
  if(!result.empty())result+=" ";
  result+=chunk;
  if(before==ptr)break;
 }
 return result;
}
#endif
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
const char* classicVoiceProfile(int language){
 // One canonical mapping for asset validation and eSpeak fallback selection.
 // Russian is index 4 internally (UI index 5; index 4 in the UI is Retro).
 static const char* const profiles[]={"gmw/en","gmw/de","roa/fr","roa/es","zle/ru"};
 return (language>=0&&language<5)?profiles[language]:nullptr;
}
std::string neuralPreviewFilename(int language,bible::Ref ref){
 if(language!=1||ref.book!=0||ref.chapter!=1||ref.verse<1||ref.verse>31)return {};
 return "neural/de/genesis1/"+std::to_string(ref.verse)+".pcm";
}
static bool readNeuralPreview(const std::string& relative,std::vector<short>& pcm){
 if(relative.empty())return false;
 const char* roots[]={"/app0/assets/tts/","/data/homebrew/PPSA99793/assets/tts/"};
 for(const char* root:roots){
  const std::string path=std::string(root)+relative;
  FILE* file=std::fopen(path.c_str(),"rb");
  if(!file)continue;
  const bool seek=std::fseek(file,0,SEEK_END)==0;
  const long length=seek?std::ftell(file):-1;
  if(length<1000||length>6*1024*1024||length%2!=0||
      std::fseek(file,0,SEEK_SET)!=0){std::fclose(file);continue;}
  std::vector<short> bytes((size_t)length/sizeof(short));
  const size_t count=std::fread(bytes.data(),sizeof(short),bytes.size(),file);
  std::fclose(file);
  if(count!=bytes.size())continue;
  pcm=std::move(bytes);
  return true;
 }
 return false;
}
bool Speaker::start(const bible::Library& lib,bible::Ref from,int lang,bool preferNeural){
 stop();
 if(!lib.verse(from)){bible_log::write("TTS_BAD_REF","book=%d chapter=%d verse=%d",from.book,from.chapter,from.verse);return false;}
 bible_log::write("TTS_START","language=%d book=%d chapter=%d verse=%d",lang,from.book,from.chapter,from.verse);
 library_=&lib;from_=from;robotPreset_=(lang==4);
 language_=robotPreset_?0:(lang==5?4:std::clamp(lang,0,3));
 preferNeural_=preferNeural&&!robotPreset_;cancelled_=false;
 if(robotPreset_)bible_log::write("RETRO_MODE","Stephen Hawking inspired retro computer voice; English; eSpeak only");
 bible_log::write("TTS_ENGINE","preferNeural=%d language=%d",preferNeural?1:0,language_);
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
#if defined(BIBLE_NATIVE_PIPER)
 bible_piper::NativeInference native;
 bool nativeReady=false;
#endif
 const char* roots[]={"/app0/assets/tts","/data/homebrew/PPSA99793/assets/tts"};
 const char* chosenRoot=nullptr;
 int rate=-1;
 const char* languages[]={"en","de","fr","es","ru"};
 for(const char* root:roots){
  const std::string data=std::string(root)+"/espeak-ng-data/";
  const long phon=bible_log::fileSize(data+"phondata");
  const long dict=bible_log::fileSize(data+languages[language_]+std::string("_dict"));
  const char* voiceProfile=classicVoiceProfile(language_);
  const long profile=voiceProfile?bible_log::fileSize(data+"lang/"+voiceProfile):-1;
  bible_log::write("TTS_PATH","root=%s phondata=%ld dictionary=%ld profile=%ld",root,phon,dict,profile);
  if(phon>0&&dict>0&&profile>0){chosenRoot=root;break;}
 }
 if(!chosenRoot){error="Offline voice data missing";bible_log::write("TTS_DATA_ERR","No complete language assets for %s",languages[language_]);goto done;}
 rate=espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS,100,chosenRoot,espeakINITIALIZE_DONT_EXIT);
 bible_log::write("TTS_INIT","sample_rate=%d root=%s",rate,chosenRoot);
 if(rate<=0){error="Offline voice initialization failed";goto done;}
 initialized=true;
 {
  const char* codes[]={"en","de","fr","es","ru"};
  const char* code=codes[language_];
  const char* profile=classicVoiceProfile(language_);
  // The PS5 eSpeak voice enumerator can return zero even with valid data.
  // Always use the corresponding name-based fallback, including zle/ru.
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
  // eSpeak is process-global: a previous Retro worker can leave modified
  // pitch/rate/range after espeak_Terminate(). Always set all three at start.
  // Classic remains deliberately available, but must sound normal again.
  const int speed=robotPreset_?145:175;
  const int pitch=robotPreset_?36:50;
  const int range=robotPreset_?1:50;
  espeak_SetParameter(espeakRATE,speed,0);
  espeak_SetParameter(espeakPITCH,pitch,0);
  espeak_SetParameter(espeakRANGE,range,0);
  bible_log::write(robotPreset_?"RETRO_READY":"TTS_PARAMS_RESET",
                   "rate=%d pitch=%d range=%d language=%d",
                   speed,pitch,range,language_);
 }
#if defined(BIBLE_NATIVE_PIPER)
 if(preferNeural_&&!robotPreset_){
  const char* filenames[]={
   "en/en_US-kristin-medium.onnx",
   "de/de_DE-thorsten-medium.onnx",
   "fr/fr_FR-siwis-medium.onnx",
   "es/es_ES-davefx-medium.onnx",
   "ru/ru_RU-denis-medium.onnx"
  };
  const std::string relative=std::string("/neural/")+filenames[language_];
  const std::string local=std::string("/app0/assets/tts")+relative;
  const std::string fallback=std::string("/data/homebrew/PPSA99793/assets/tts")+relative;
  const char* models[]={local.c_str(),fallback.c_str()};
  for(const char* model:models){
   if(bible_log::fileSize(model)<1000000)continue;
   std::string why;
   bible_log::write("PIPER_LOAD","Trying PS5 native model: %s",model);
   const auto begin=std::chrono::steady_clock::now();
   if(native.open(model,why)){
    nativeReady=true;
    bible_log::write("PIPER_READY","model loaded in %lld ms",
     (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-begin).count());
    break;
   }
   bible_log::write("PIPER_LOAD_ERR","%s",why.c_str());
  }
  if(!nativeReady)bible_log::write("PIPER_FALLBACK","Native model not initialized, classic/preview remains available");
 }
#endif
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
   if(verse->number==1||(ref.book==from_.book&&ref.chapter==from_.chapter&&ref.verse==from_.verse))
    bible_log::write("TTS_VERSE","book=%d chapter=%d verse=%d",ref.book,ref.chapter,ref.verse);
   Snapshot st{};st.active=true;st.ref=ref;status(st);
   auto result=espeak_Synth(phrase_.c_str(),phrase_.size()+1,0,POS_CHARACTER,0,
                            espeakCHARS_UTF8,nullptr,nullptr);
   if(cancelled_)break;
   if(result!=EE_OK||pcm_.empty()){error="Offline synthesis failed";
    bible_log::write("TTS_SYNTH_ERR","rc=%d pcm_samples=%zu book=%d chapter=%d verse=%d",(int)result,pcm_.size(),ref.book,ref.chapter,ref.verse);
    break;}
   // Priority: experimental on-console Piper -> pre-rendered Genesis 1
   // preview -> classic eSpeak. All paths retain the same controller/SDL UI.
   const std::string neuralFile=preferNeural_?neuralPreviewFilename(language_,ref):std::string{};
   bool usingNeural=false;
   int inputRate=rate;
#if defined(BIBLE_NATIVE_PIPER)
   if(preferNeural_&&nativeReady&&!cancelled_){
    const auto started=std::chrono::steady_clock::now();
    const std::string ipa=ipaForPiper(phrase_);
    const bible_piper::PhonemeTable table=
     language_==0?bible_piper::voice_en::thorstenTable():
     language_==1?bible_piper::thorstenTable():
     language_==2?bible_piper::voice_fr::thorstenTable():
     language_==3?bible_piper::voice_es::thorstenTable():
                  bible_piper::voice_ru::thorstenTable();
    const auto ids=bible_piper::encode(ipa,table);
    if(ids.success){
     std::vector<std::int16_t> generated;
     std::string why;
     if(native.synthesize(ids.ids,generated,why)&&!cancelled_){
      const auto oldSamples=pcm_.size();
      if(oldSamples)for(auto& mark:marks_)
       mark.timeMs=(int)std::lround((double)mark.timeMs*generated.size()/oldSamples);
      pcm_.assign(generated.begin(),generated.end());
      usingNeural=true;inputRate=22050;
      bible_log::write("PIPER_NATIVE","book=%d chapter=%d verse=%d samples=%zu ms=%lld",
       ref.book,ref.chapter,ref.verse,pcm_.size(),
       (long long)std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now()-started).count());
     }else{
      bible_log::write("PIPER_RUN_ERR","%s",why.c_str());
     }
    }else{
     bible_log::write("PIPER_IPA_ERR","%s",ids.error.c_str());
    }
   }
#endif
   if(!usingNeural&&!neuralFile.empty()){
    std::vector<short> highQuality;
    if(readNeuralPreview(neuralFile,highQuality)){
     const auto priorSamples=pcm_.size();
     // eSpeak word timings are used only as an approximation for Piper audio.
     // Rescale onset times to the different total audio duration.
     if(priorSamples>0)for(auto& mark:marks_){
      mark.timeMs=(int)std::lround((double)mark.timeMs*
                                 (double)highQuality.size()/(double)priorSamples);
     }
     pcm_=std::move(highQuality);inputRate=22050;usingNeural=true;
     bible_log::write("PIPER_PREVIEW","sample=%s samples=%zu",neuralFile.c_str(),pcm_.size());
    }else bible_log::write("PIPER_FALLBACK","missing/corrupt=%s (using classic)",neuralFile.c_str());
   }
   st.neural=usingNeural;
   SDL_AudioCVT cvt{};
   if(SDL_BuildAudioCVT(&cvt,AUDIO_S16SYS,1,inputRate,AUDIO_S16SYS,channels,outputRate)<0){
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
