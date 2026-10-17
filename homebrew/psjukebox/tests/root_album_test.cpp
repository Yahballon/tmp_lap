// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/library.hpp"
#include <cassert>
#include <cstdio>
int main(int argc,char** argv){
 assert(argc==2);
 auto files=jukebox::find_packed_albums({argv[1]});
 assert(files.size()==1);
 assert(files[0].artist.empty());
 assert(files[0].size>0);
 assert(files[0].path.find("Demo.album")!=std::string::npos);
 std::puts("PASS Music/ root .album discovery without artist directory");
}
