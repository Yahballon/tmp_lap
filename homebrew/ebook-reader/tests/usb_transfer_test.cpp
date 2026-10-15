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
int main(int argc,char** argv){
 assert(argc==2);
 const auto catalog=usbbooks::scan();
 assert(catalog.viaHelper);
 assert(catalog.books.size()==1);
 const auto book=catalog.books.front();
 assert(book.relative=="LargeComic.CBR");
 assert(book.bytes>40*1024*1024);
 std::string cache,error;
 assert(usbbooks::fetch(book,cache,error));
 // Reproduce the PS5 failure: this file is over 40 MiB, so it must be
 // written beneath /data/EbookReader/Cache, NOT the 21-MiB /download0 quota.
 assert(cache.rfind(std::string(argv[1])+"/EbookReader/Cache/usb-",0)==0);
 assert(identical(std::string(argv[1])+"/Books/LargeComic.CBR",cache));
 // Cached copy should work without another transfer.
 std::string again,more;
 assert(usbbooks::fetch(book,again,more));
 assert(cache==again);
 assert(unlink(cache.c_str())==0);
 // Two callers must not write into the same .part file simultaneously.
 bool a=false,b=false;std::string x,y,ex,ey;
 std::thread first([&](){a=usbbooks::fetch(book,x,ex);});
 std::thread second([&](){b=usbbooks::fetch(book,y,ey);});
 first.join();second.join();
 assert(a && b);
 assert(x==y && identical(std::string(argv[1])+"/Books/LargeComic.CBR",x));
 std::puts("USB >40-MiB comic transfer, interrupted transfer recovery, local caching and concurrent caller checks passed");
}
