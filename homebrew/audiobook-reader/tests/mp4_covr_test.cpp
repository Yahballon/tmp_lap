#include "../native/mp4_cover.hpp"
#include "../native/audio_cover_store.hpp"
#include <cassert>
#include <cstdio>
#include <string>
int main(int argc,char** argv){
 assert(argc==3);
 std::string value;
 assert(audio_cover::extract_mp4_covr(argv[1],value));
 assert(audio_cover::image(value));
 assert(value.size()>100);
 value="bad";
 assert(!audio_cover::extract_mp4_covr(argv[2],value));
 assert(value.empty());
 puts("PASS: nested iTunes moov/udta/meta/ilst/covr/data cover");
}
