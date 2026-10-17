// SPDX-License-Identifier: GPL-3.0-or-later
// Reproduce the tiny pthread stack used for background album scans on PS5.
// Previous write_entry() allocated two 32768-byte arrays on this stack.
// A valid compressed .album would SIGSEGV rather than completing its import.
#include "../native/album_archive.hpp"
#include "../native/library.hpp"
#include <pthread.h>
#include <cassert>
#include <cstdio>
#include <string>
struct Arguments {std::string archive,cache,error; bool ok=false;};
void* scan(void* raw) {
 auto& args=*static_cast<Arguments*>(raw);
 std::string artist=jukebox::album_artist_marker(args.archive,args.error);
 if(artist!="Tupac")return nullptr;
 std::string root;
 if(!jukebox::unpack_album(args.archive,artist,"test-release",
                           "small-stack-test",args.cache,root,args.error))return nullptr;
 auto lib=jukebox::scan({root});
 args.ok=lib.trackCount==2&&lib.artists.size()==1&&
         lib.artists[0].albums.size()==1&&
         !lib.artists[0].albums[0].background.empty()&&
         !lib.artists[0].albums[0].cover.empty();
 return nullptr;
}
int main(int argc,char** argv) {
 assert(argc==3);
 Arguments args{argv[1],argv[2],"",false};
 pthread_attr_t attr;assert(pthread_attr_init(&attr)==0);
 assert(pthread_attr_setstacksize(&attr,64*1024)==0);
 pthread_t worker{};
 assert(pthread_create(&worker,&attr,&scan,&args)==0);
 pthread_attr_destroy(&attr);
 assert(pthread_join(worker,nullptr)==0);
 if(!args.ok)fprintf(stderr,"FAILED 64 KiB album scan: %s\n",args.error.c_str());
 assert(args.ok);
 std::puts("PASS .album import/extract and artwork scan on 64 KiB thread stack");
}
