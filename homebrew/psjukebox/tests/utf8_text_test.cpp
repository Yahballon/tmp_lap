// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/utf8_text.hpp"
#include <cassert>
#include <cstdio>
#include <string>
int main(){
 const std::string latin="Björk – Künstler Äpfel süß";
 const std::string greek="Καλημέρα κόσμε";
 const std::string cyrillic="Привет, музыка";
 const std::string asian="東京と音楽 カタカナ";
 assert(jukebox::utf8_cut(latin,90)==latin);
 assert(jukebox::utf8_cut(greek,90)==greek);
 assert(jukebox::utf8_cut(cyrillic,90)==cyrillic);
 assert(jukebox::utf8_cut(asian,90)==asian);
 assert(jukebox::utf8_cut(latin,8)=="Björk...");
 assert(jukebox::utf8_cut(cyrillic,6)=="При...");
 assert(jukebox::utf8_cut(asian,5)=="東京...");
 assert(jukebox::utf8_cut(latin,0).empty());
 puts("PASS UTF-8 titles remain complete when cut; umlauts, accents, Greek, Cyrillic, Japanese");
}
