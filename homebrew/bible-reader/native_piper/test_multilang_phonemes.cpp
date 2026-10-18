// Offline Piper voice table regression test: generated from original model JSON.
// GPL-3.0-or-later
#include "thorsten_phonemes.generated.hpp"
#include "voice_en_phonemes.generated.hpp"
#include "voice_fr_phonemes.generated.hpp"
#include "voice_es_phonemes.generated.hpp"
#include "voice_ru_phonemes.generated.hpp"
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <vector>
int main(){
 const bible_piper::PhonemeTable tables[]={
  bible_piper::voice_en::thorstenTable(),
  bible_piper::thorstenTable(),
  bible_piper::voice_fr::thorstenTable(),
  bible_piper::voice_es::thorstenTable(),
  bible_piper::voice_ru::thorstenTable()
 };
 for(const auto& table:tables){
  assert(table.symbols && table.values && table.symbolsCount>=140);
  const auto result=bible_piper::encode("a b",table);
  assert(result.success);
  assert(result.ids.front()==1 && result.ids.back()==2);
  assert(!bible_piper::encode("Ω",table).success);
 }
 std::puts("Piper EN/DE/FR/ES/RU generated voice tables compile and pass decoding tests");
 return 0;
}
