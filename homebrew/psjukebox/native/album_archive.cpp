// SPDX-License-Identifier: GPL-3.0-or-later
#include "album_archive.hpp"
#include "library.hpp"
#include <zlib.h>
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <climits>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
namespace jukebox {
namespace {
constexpr uint64_t maxPack=1024ULL*1024*1024; // 1 GiB compressed file
constexpr uint64_t maxExpanded=2ULL*1024*1024*1024;
constexpr uint64_t maxTrack=512ULL*1024*1024;
constexpr uint64_t maxArt=12ULL*1024*1024;
struct Entry{
 std::string name;
 uint32_t crc=0,compressed=0,plain=0,local=0;
 uint16_t method=0,flags=0;
};
uint16_t u16(const unsigned char* p){return uint16_t(p[0])|(uint16_t(p[1])<<8);}
uint32_t u32(const unsigned char* p){return uint32_t(u16(p))|(uint32_t(u16(p+2))<<16);}
std::string lower(std::string s){
 for(char& c:s)c=(char)std::tolower((unsigned char)c);
 return s;
}
bool safe(const std::string& s){
 if(s.empty()||s.size()>180||s=="."||s==".."||s[0]=='.')return false;
 for(unsigned char c:s)if(c<32||c==127||c=='/'||c=='\\')return false;
 return true;
}
bool seek(FILE* f,uint64_t pos){return pos<=LONG_MAX&&fseek(f,(long)pos,SEEK_SET)==0;}
bool exact(FILE* f,void* p,size_t count){return fread(p,1,count,f)==count;}
bool dir(const std::string& p){
 struct stat st{};
 if(lstat(p.c_str(),&st)==0)return S_ISDIR(st.st_mode);
 return mkdir(p.c_str(),0755)==0;
}
uint64_t hash(const std::string& path){
 uint64_t h=14695981039346656037ULL;
 for(unsigned char c:path){h^=c;h*=1099511628211ULL;}
 return h;
}
std::string hex(uint64_t n){
 char b[20];snprintf(b,sizeof(b),"%016llx",(unsigned long long)n);return b;
}
bool wanted(const std::string& name){
 if(is_music(name))return true;
 const std::string s=lower(name);
 return s=="cover.png"||s=="cover.jpg"||s=="cover.jpeg"||
        s=="background.png"||s=="background.jpg"||s=="background.jpeg";
}
bool write_entry(FILE* zip,const Entry& e,const std::string& destination,
                 uint64_t limit,std::string& error){
 unsigned char lh[30]{};
 if(!seek(zip,e.local)||!exact(zip,lh,sizeof(lh))||u32(lh)!=0x04034b50u){
  error="Invalid ZIP local header";return false;
 }
 if(u16(lh+8)!=e.method||u16(lh+6)&1){
  error="ZIP local entry mismatch/encrypted";return false;
 }
 const uint64_t data=e.local+30ULL+u16(lh+26)+u16(lh+28);
 if(!seek(zip,data)){error="Invalid ZIP member offset";return false;}
 FILE* out=fopen((destination+".part").c_str(),"wb");
 if(!out){error="Cannot write album cache: "+std::to_string(errno);return false;}
 bool okay=true;
 uint64_t bytesLeft=e.compressed,plain=0;
 uLong crc=crc32(0L,Z_NULL,0);
 unsigned char input[32768],output[32768];
 z_stream z{};
 bool deflate=e.method==8,finished=false;
 if(deflate&&inflateInit2(&z,-MAX_WBITS)!=Z_OK)okay=false;
 while(okay&&bytesLeft){
  const size_t n=(size_t)std::min<uint64_t>(bytesLeft,sizeof(input));
  if(!exact(zip,input,n)){okay=false;error="Truncated album member";break;}
  bytesLeft-=n;
  if(!deflate){
   plain+=n;
   if(plain>limit||fwrite(input,1,n,out)!=n){okay=false;error="Album file size/write limit";break;}
   crc=crc32(crc,input,(uInt)n);
   continue;
  }
  z.next_in=input;z.avail_in=(uInt)n;
  while(z.avail_in&&okay){
   z.next_out=output;z.avail_out=sizeof(output);
   const int rc=inflate(&z,Z_NO_FLUSH);
   const size_t got=sizeof(output)-z.avail_out;
   plain+=got;
   if(plain>limit||(got&&fwrite(output,1,got,out)!=got)){
    okay=false;error="Album inflate size/write limit";break;
   }
   if(got)crc=crc32(crc,output,(uInt)got);
   if(rc==Z_STREAM_END){finished=true;break;}
   if(rc!=Z_OK||(got==0&&z.avail_in==0&&bytesLeft==0)){
    okay=false;error="Invalid deflate stream";break;
   }
  }
  if(finished&&bytesLeft){okay=false;error="ZIP compressed size mismatch";}
 }
 if(deflate){inflateEnd(&z);if(!finished)okay=false;}
 if(plain!=e.plain||crc!=e.crc){okay=false;error="ZIP CRC/size mismatch";}
 if(fflush(out)!=0)okay=false;
 if(fclose(out)!=0)okay=false;
 const std::string part=destination+".part";
 if(!okay){remove(part.c_str());return false;}
 if(rename(part.c_str(),destination.c_str())!=0){
  remove(part.c_str());error="Cannot finalize archive file";return false;
 }
 return true;
}
}
bool is_album_file(const std::string& name){
 auto s=lower(name);
 return s.size()>6&&s.substr(s.size()-6)==".album";
}
std::string album_stem(const std::string& filename){
 auto slash=filename.find_last_of("/\\");
 const auto base=slash==std::string::npos?filename:filename.substr(slash+1);
 if(!is_album_file(base))return {};
 const auto stem=base.substr(0,base.size()-6);
 return safe(stem)?stem:std::string{};
}
std::string archive_cache_root(){
#ifdef JUKEBOX_DATA_ROOT
 return std::string(JUKEBOX_DATA_ROOT)+"/AlbumCache";
#else
 return "/data/PSJukebox/AlbumCache";
#endif
}
bool unpack_album(const std::string& path,const std::string& artist,
                  const std::string& album,const std::string& sourceKey,
                  const std::string& cacheRoot,std::string& musicRoot,
                  std::string& error){
 musicRoot.clear();error.clear();
 if(!safe(artist)||!safe(album)||sourceKey.empty()){error="Invalid album/artist name";return false;}
 struct stat st{};
 if(lstat(path.c_str(),&st)!=0||!S_ISREG(st.st_mode)||st.st_size<22||
    (uint64_t)st.st_size>maxPack){error="Invalid or oversized .album file";return false;}
 const uint64_t length=(uint64_t)st.st_size;
 // A content-size-qualified cache directory isolates different ZIP files
 // while preserving the original Artist/Album layout for the library scanner.
 const std::string id=hex(hash(sourceKey+"#"+std::to_string(length)));
 const std::string home=cacheRoot+"/"+id;
 musicRoot=home+"/Music";
 const std::string folder=musicRoot+"/"+artist+"/"+album;
 const std::string marker=folder+"/.album-ready";
 if(access(marker.c_str(),R_OK)==0)return true;
 FILE* z=fopen(path.c_str(),"rb");
 if(!z){error="Could not open album archive";return false;}
 bool okay=false;
 do{
  const size_t tailSize=(size_t)std::min<uint64_t>(length,65557);
  std::vector<unsigned char> tail(tailSize);
  if(!seek(z,length-tailSize)||!exact(z,tail.data(),tail.size())){error="ZIP tail unavailable";break;}
  size_t eocd=tailSize;
  for(size_t i=tailSize-22;;i--){
   if(u32(tail.data()+i)==0x06054b50u){eocd=i;break;}
   if(i==0)break;
  }
  if(eocd==tailSize){error="ZIP central directory missing";break;}
  const unsigned char* e=tail.data()+eocd;
  if(u16(e+4)!=0||u16(e+6)!=0||u16(e+8)!=u16(e+10)||
     u16(e+10)>512||u16(e+10)==0xffff||u32(e+12)==0xffffffffu||
     u32(e+16)==0xffffffffu){error="Unsupported multi-disk/ZIP64 album";break;}
  const uint64_t center=u32(e+16),end=center+u32(e+12);
  if(end>length||u16(e+20)+eocd+22>tailSize){error="Invalid ZIP directory bounds";break;}
  if(!seek(z,center)){error="Cannot seek ZIP directory";break;}
  std::vector<Entry> files;
  uint64_t expanded=0;
  bool invalid=false;
  for(unsigned i=0;i<u16(e+10);i++){
   unsigned char h[46]{};
   if(!exact(z,h,sizeof(h))||u32(h)!=0x02014b50u){invalid=true;break;}
   const unsigned n=u16(h+28),extra=u16(h+30),comment=u16(h+32);
   if(n==0||n>360||extra>65535||comment>65535){invalid=true;break;}
   std::string name(n,'\0');
   if(!exact(z,name.data(),n)||fseek(z,(long)(extra+comment),SEEK_CUR)!=0){invalid=true;break;}
   const uint32_t compressed=u32(h+20),plain=u32(h+24);
   // Directory/symlink entries are never extracted, and only supported media
   // files are accepted, at archive root or inside ONE wrapper folder.
   const auto slash=name.find('/');
   std::string base=name;
   if(slash!=std::string::npos){
    if(!safe(name.substr(0,slash))||name.find('/',slash+1)!=std::string::npos)continue;
    base=name.substr(slash+1);
   }
   if(!safe(base)||!wanted(base))continue;
   const auto method=u16(h+10),flags=u16(h+8);
   const unsigned unixMode=u32(h+38)>>16;
   if((unixMode&0170000)==0120000){invalid=true;error="Symlink in album";break;}
   if(flags&1||(method!=0&&method!=8)||compressed==0||u32(h+42)>=length||
      plain>(is_music(base)?maxTrack:maxArt)||uint64_t(plain)>maxExpanded-expanded||
      (compressed>0&&uint64_t(plain)>uint64_t(compressed)*1500)){
    invalid=true;error="Unsafe/unsupported ZIP member";break;
   }
   expanded+=plain;
   files.push_back({base,u32(h+16),compressed,plain,u32(h+42),method,flags});
  }
  if(invalid||files.empty()){if(error.empty())error="Invalid or empty packed album";break;}
  if(!dir(cacheRoot)||!dir(home)||!dir(musicRoot)||!dir(musicRoot+"/"+artist)||!dir(folder)){
   error="Could not create album cache";break;
  }
  bool extracted=true;
  for(const auto& entry:files){
   if(!write_entry(z,entry,folder+"/"+entry.name,
                   is_music(entry.name)?maxTrack:maxArt,error)){extracted=false;break;}
  }
  if(!extracted)break;
  FILE* done=fopen(marker.c_str(),"wb");
  if(!done){error="Cannot write archive-ready marker";break;}
  fprintf(done,"ZIP %llu entries=%zu\n",(unsigned long long)length,files.size());
  fclose(done);okay=true;
 }while(false);
 fclose(z);
 return okay;
}
}
