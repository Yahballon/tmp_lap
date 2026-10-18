// SPDX-License-Identifier: GPL-3.0-or-later
// PS5 reports EPERM for opendir() on app-owned AlbumCache/Music while
// the archive can still be extracted/opened. Index directly from ZIP entries.
#include "../native/album_archive.hpp"
#include "../native/library.hpp"
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <string>
extern "C" DIR* __real_opendir(const char*);
extern "C" DIR* __wrap_opendir(const char* path){
 if(std::strstr(path,"/AlbumCache/")!=nullptr){
  errno=EPERM;return nullptr;
 }
 return __real_opendir(path);
}
int main(int argc,char** argv){
 assert(argc==3);
 std::string root,error;
 jukebox::Library catalog;
 assert(jukebox::unpack_album(argv[1],"Tupac","Demo","jbox://8/Demo.album",
                             argv[2],root,error,&catalog));
 assert(catalog.trackCount==2);
 assert(catalog.artists.size()==1);
 const auto& albums=catalog.artists[0].albums;
 assert(albums.size()==1);
 assert(albums[0].tracks.size()==2);
 assert(!albums[0].cover.empty());
 assert(!albums[0].background.empty());
 // This is the console bug: ordinary filesystem enumeration is denied.
 auto inaccessible=jukebox::scan({root});
 assert(inaccessible.trackCount==0);
 assert(inaccessible.artists.empty());
 // Every media file in the imported index has an actual readable cache file.
 for(const auto& track:albums[0].tracks){
  FILE* f=fopen(track.path.c_str(),"rb");
  assert(f);assert(fgetc(f)!=EOF);fclose(f);
 }
 FILE* cover=fopen(albums[0].cover.c_str(),"rb");
 assert(cover);fclose(cover);
 FILE* background=fopen(albums[0].background.c_str(),"rb");
 assert(background);fclose(background);
 // Cache reuse must still synthesize an album catalog, not silently return.
 jukebox::Library repeat;
 std::string again,err2;
 assert(jukebox::unpack_album(argv[1],"Tupac","Demo","jbox://8/Demo.album",
                             argv[2],again,err2,&repeat));
 assert(again==root);
 assert(repeat.trackCount==2);
 std::puts("PASS Music-root .album index without opendir, reusable cache and playable paths");
}
