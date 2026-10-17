// GPL-3.0-or-later. Portable Piper IPA->input tensor test.
#include "thorsten_phonemes.generated.hpp" // Includes phoneme_ids.hpp from the same build root
#include <cassert>
#include <cstdint>
#include <string>
int main(){
 const auto table=bible_piper::thorstenTable();
 assert(table.symbolsCount>=140);
 const auto result=bible_piper::encode("a b",table);
 assert(result.success);
 // From Thorsten Medium's real symbol map: ^ = 1, _ = 0, a = 14,
 // space = 3, b = 15 and $ = 2.
 assert((result.ids==std::vector<std::int64_t>{1,0,14,0,3,0,15,0,2}));
 assert(!bible_piper::encode("",table).success);
 assert(!bible_piper::encode(std::string("\xff",1),table).success);
 assert(!bible_piper::encode("Ω",table).success);
 const auto stress=bible_piper::encode("ʃ",table);
 assert(stress.success);
 assert((stress.ids==std::vector<std::int64_t>{1,0,96,0,2}));
}
