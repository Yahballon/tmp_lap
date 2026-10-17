// GPL-3.0-or-later
#include "../native/bible.hpp"
#include "../native/tts_reader.hpp"
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
 std::puts("TTS Unicode offsets and Bible end bounds passed");
}
