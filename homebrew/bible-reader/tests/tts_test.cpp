// GPL-3.0-or-later
#include "../native/bible.hpp"
#include "../native/tts_reader.hpp"
#include "../native/localization.hpp"
#include <cassert>
#include <cstdio>
#include <string>
int main(){
 const std::string sentence="Äpfel noël €";
 assert(bible_tts::utf8Offset(sentence,0)==0);
 assert(bible_tts::utf8Offset(sentence,1)==2);
 assert(bible_tts::utf8Offset(sentence,5)==6);
 assert(bible_tts::utf8Offset(sentence,100)==sentence.size());
 bible::Library empty;bible::Ref start{0,1,1};
 assert(!bible_tts::nextVerse(empty,start));
 assert(bible_tts::neuralPreviewFilename(1,{0,1,1})=="neural/de/genesis1/1.pcm");
 assert(bible_tts::neuralPreviewFilename(1,{0,1,31})=="neural/de/genesis1/31.pcm");
 assert(bible_tts::neuralPreviewFilename(1,{0,1,32}).empty());
 assert(bible_tts::neuralPreviewFilename(0,{0,1,1}).empty());
 assert(bible_tts::neuralPreviewFilename(1,{0,2,1}).empty());
 assert(bible_i18n::kLanguageOptions==6);
 assert(std::string(bible_i18n::code(5))=="ru");
 // Regression: Russian is the fifth real voice (internal index 4), while
 // language option 4 is the English Retro mode.
 const char* profiles[]={"gmw/en","gmw/de","roa/fr","roa/es","zle/ru"};
 for(int i=0;i<5;++i)
  assert(std::string(bible_tts::classicVoiceProfile(i))==profiles[i]);
 assert(bible_tts::classicVoiceProfile(-1)==nullptr);
 assert(bible_tts::classicVoiceProfile(5)==nullptr);
 std::puts("TTS Unicode offsets and Bible end bounds passed");
}
