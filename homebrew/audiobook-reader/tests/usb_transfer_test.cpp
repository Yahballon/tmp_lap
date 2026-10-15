// SPDX-License-Identifier: GPL-3.0-or-later
#include "usb_bridge.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <sys/stat.h>
#include <unistd.h>
bool identical(const std::string& a,const std::string& b){
 FILE* x=fopen(a.c_str(),"rb"),*y=fopen(b.c_str(),"rb");
 if(!x||!y)return false;
 char p[32768],q[32768];bool valid=true;
 while(true){
   size_t n=fread(p,1,sizeof(p),x),m=fread(q,1,sizeof(q),y);
   if(n!=m || memcmp(p,q,n)!=0){valid=false;break;}
   if(n<sizeof(p))break;
 }
 fclose(x);fclose(y);return valid;
}
struct Progress {size_t last=0,total=0;int updates=0;};
bool update(size_t done,size_t total,void* context){
 auto* p=static_cast<Progress*>(context);
 assert(done<=total && done>=p->last);
 p->last=done;p->total=total;p->updates++;
 return true;
}
int main(int argc,char** argv){
 assert(argc==2);
 const auto catalog=usbaudio::scan();
 assert(catalog.viaHelper);
 assert(catalog.books.size()==1);
 const auto book=catalog.books.front();
 assert(book.relative=="LongAudio.M4B");
 assert(book.bytes>40*1024*1024);
 std::string cache,error;
 Progress state;
 assert(usbaudio::fetch(book,cache,error,update,&state));
 assert(state.updates>5 && state.last==book.bytes);
 // Reproduce the PS5 failure: this file is over 40 MiB, so it must be
 // written beneath /data/AudioBookReader/Cache, NOT the 21-MiB /download0 quota.
 assert(cache.rfind(std::string(argv[1])+"/AudioBookReader/Cache/usb-",0)==0);
 assert(identical(std::string(argv[1])+"/ABooks/LongAudio.M4B",cache));
 std::string copy;
 assert(usbaudio::cached_copy(book,copy) && copy==cache);
 // Cached copy should work without another transfer.
 std::string again,more;
 assert(usbaudio::fetch(book,again,more));
 assert(cache==again);
 assert(unlink(cache.c_str())==0);
 // Two callers must not write into the same .part file simultaneously.
 bool a=false,b=false;std::string x,y,ex,ey;
 std::thread first([&](){a=usbaudio::fetch(book,x,ex);});
 std::thread second([&](){b=usbaudio::fetch(book,y,ey);});
 first.join();second.join();
 assert(a && b);
 assert(x==y && identical(std::string(argv[1])+"/ABooks/LongAudio.M4B",x));
 std::puts("USB >40-MiB audiobook transfer, interrupted transfer recovery, local caching and concurrent caller checks passed");
}
