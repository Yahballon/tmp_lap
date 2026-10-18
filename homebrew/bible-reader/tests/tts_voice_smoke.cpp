// SPDX-License-Identifier: GPL-3.0-or-later
// Test real compiled offline eSpeak NG language data and spoken PCM.
#include <espeak-ng/speak_lib.h>
#include <cstdio>
#include <cstdlib>
#include <string>
namespace {
int soundSamples=0;
int callback(short* sound,int n,espeak_EVENT*){
 if(sound&&n>0)soundSamples+=n;
 return 0;
}
}
int main(int argc,char** argv){
 if(argc!=2)return 2;
 const int rate=espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS,100,argv[1],
                                  espeakINITIALIZE_DONT_EXIT);
 if(rate<=0){std::fprintf(stderr,"Initialization failed: %d\n",rate);return 3;}
 espeak_SetSynthCallback(callback);
 const char* languages[]={"en","de","fr","es","ru"};
 const char* profiles[]={"gmw/en","gmw/de","roa/fr","roa/es","zle/ru"};
 const char* texts[]={"In the beginning, God created the heavens and the earth.",
   "Am Anfang schuf Gott Himmel und Erde.",
   "Au commencement, Dieu créa les cieux et la terre.",
   "En el principio creó Dios los cielos y la tierra.",
   "В начале сотворил Бог небо и землю."};
 for(int i=0;i<5;++i){
  espeak_VOICE spec{};spec.languages=const_cast<char*>(languages[i]);
  const auto select=espeak_SetVoiceByProperties(&spec);
  if(select!=EE_OK){std::fprintf(stderr,"Cannot select %s: %d\n",languages[i],(int)select);return 4;}
  // PS5's espeak_ListVoices() sometimes lists zero languages even though
  // language profile data is present. Test the exact production fallback.
  const auto byName=espeak_SetVoiceByName(profiles[i]);
  if(byName!=EE_OK){
   std::fprintf(stderr,"Cannot select fallback %s: %d\n",profiles[i],(int)byName);
   return 6;
  }
  soundSamples=0;
  const std::string passage=texts[i];
  const auto rc=espeak_Synth(passage.c_str(),passage.size()+1,0,POS_CHARACTER,0,espeakCHARS_UTF8,nullptr,nullptr);
  if(rc!=EE_OK||soundSamples<1000){std::fprintf(stderr,"No speech generated: %s rc=%d samples=%d\n",languages[i],(int)rc,soundSamples);return 5;}
  std::printf("PASS voice %s sample_rate=%d audio_samples=%d\n",languages[i],rate,soundSamples);
 }
 espeak_Terminate();
 return 0;
}
