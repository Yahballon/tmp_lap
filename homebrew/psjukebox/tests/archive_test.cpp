// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/album_archive.hpp"
#include "../native/library.hpp"
#include <cassert>
#include <cstdio>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
int main(int argc,char** argv){
 assert(argc==3);
 assert(jukebox::is_album_file("demo.album"));
 assert(jukebox::is_album_file("DREAM.ALBUM"));
 assert(!jukebox::is_album_file("demo.zip"));
 assert(jukebox::album_stem("jbox://8/Tupac/Demo.album")=="Demo");
 std::string root,error;
 assert(jukebox::unpack_album(argv[1],"Tupac","Killuminati","jbox://8/Tupac/Demo.album",
                              argv[2],root,error));
 auto lib=jukebox::scan({root});
 assert(lib.artists.size()==1);
 assert(lib.artists[0].name=="Tupac");
 assert(lib.artists[0].albums.size()==1);
 assert(lib.artists[0].albums[0].name=="Killuminati");
 assert(lib.artists[0].albums[0].tracks.size()==2);
 assert(!lib.artists[0].albums[0].cover.empty());
 assert(!lib.artists[0].albums[0].background.empty());
 assert(lib.trackCount==2);
 std::string root2,error2;
 assert(jukebox::unpack_album(argv[1],"Tupac","Killuminati","jbox://8/Tupac/Demo.album",argv[2],root2,error2));
 assert(root==root2);
 struct stat statbuf{};
 assert(lstat((std::string(argv[2])+"/evil.mp3").c_str(),&statbuf)!=0);
 assert(!jukebox::unpack_album(argv[1],"../outside","Demo","bad",argv[2],root2,error2));
 std::puts("PASS .album ZIP deflate, stored tracks, nested wrapper, artwork, metadata filtering, cache reuse");
 return 0;
}
