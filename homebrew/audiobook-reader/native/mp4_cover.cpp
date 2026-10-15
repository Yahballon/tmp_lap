// SPDX-License-Identifier: GPL-3.0-or-later
#include "mp4_cover.hpp"
#include "audio_cover_store.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
namespace audio_cover {
namespace {
uint32_t be32(const unsigned char* p){
 return (uint32_t(p[0])<<24)|(uint32_t(p[1])<<16)|(uint32_t(p[2])<<8)|uint32_t(p[3]);
}
uint64_t be64(const unsigned char* p){return (uint64_t(be32(p))<<32)|be32(p+4);}
bool at(FILE* f,uint64_t p,void* data,size_t n,uint64_t fileSize){
 if(p>fileSize||n>fileSize-p)return false;
 if(p>INT64_MAX||fseeko(f,(off_t)p,SEEK_SET)!=0)return false;
 return fread(data,1,n,f)==n;
}
bool find(FILE* f,uint64_t start,uint64_t end,uint64_t total,int depth,
          bool inCovr,std::string& output){
 if(depth>9||end>total)return false;
 unsigned guard=0;
 while(start+8<=end && guard++<50000){
  unsigned char h[16]{};
  if(!at(f,start,h,8,total))return false;
  uint64_t size=be32(h);unsigned offset=8;
  if(size==1){if(!at(f,start+8,h+8,8,total))return false;size=be64(h+8);offset=16;}
  if(size==0)size=end-start;
  if(size<offset||size>end-start)return false;
  const uint64_t body=start+offset,finish=start+size;
  const bool moov=memcmp(h+4,"moov",4)==0;
  const bool udta=memcmp(h+4,"udta",4)==0;
  const bool meta=memcmp(h+4,"meta",4)==0;
  const bool ilst=memcmp(h+4,"ilst",4)==0;
  const bool covr=memcmp(h+4,"covr",4)==0;
  const bool data=memcmp(h+4,"data",4)==0;
  if(inCovr&&data&&size>=offset+8+8){
   // iTunes data full box: 4-byte data type + 4-byte locale.
   const uint64_t payload=body+8,length=finish-payload;
   if(length>=8 && length<=6*1024*1024){
    std::string encoded((size_t)length,'\0');
    if(at(f,payload,&encoded[0],encoded.size(),total)&&audio_cover::image(encoded)){
     output=std::move(encoded);return true;
    }
   }
  }
  if(moov||udta||meta||ilst||covr){
   uint64_t nested=body+(meta?4:0);
   if(nested<=finish && find(f,nested,finish,total,depth+1,inCovr||covr,output))
    return true;
  }
  if(start+size<=start)break;
  start+=size;
 }
 return false;
}
}
bool extract_mp4_covr(const std::string& filename,std::string& cover){
 cover.clear();
 struct stat st{};
 if(stat(filename.c_str(),&st)!=0||st.st_size<8)return false;
 FILE* f=fopen(filename.c_str(),"rb");if(!f)return false;
 const bool ok=find(f,0,(uint64_t)st.st_size,(uint64_t)st.st_size,0,false,cover);
 fclose(f);return ok;
}
}
