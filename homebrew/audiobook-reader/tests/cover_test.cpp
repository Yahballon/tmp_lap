// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/cover_extract.hpp"
#include "../native/audio_cover_store.hpp"
#include <cassert>
#include <cstdio>
#include <string>
int main(int argc,char** argv){
 assert(argc==3);
 const std::string good=argv[1],noart=argv[2];
 std::string encoded,reason;
 assert(audio_cover::extract(good,encoded,reason));
 assert(audio_cover::image(encoded));
 assert(audio_cover::save("fixture-cover",encoded));
 std::string cached;assert(audio_cover::load("fixture-cover",cached));
 assert(encoded==cached);
 assert(!audio_cover::extract(noart,encoded,reason));
 assert(encoded.empty());
 puts("PASS: M4B embedded artwork extraction and persistent Covers cache");
}
