// SPDX-License-Identifier: GPL-3.0-or-later
// LauncherX-style USB bridge: dedicated read-only ELF helper via elfldr :9021.
// The ELF helper lists local Books that cannot be enumerated by the sandbox.
#include "usb_bridge.hpp"
#include "audio_log.hpp"
#include "audio_cache.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstdint>
#include <mutex>
#include <string>

namespace usbaudio {
namespace {
#ifndef AUDIOBOOK_USB_BRIDGE_PORT
#define AUDIOBOOK_USB_BRIDGE_PORT 9179
#endif
#ifndef AUDIOBOOK_USB_CACHE_ROOT
#define AUDIOBOOK_USB_CACHE_ROOT AUDIOBOOK_DATA_ROOT "/Cache"
#endif
constexpr int kPort=AUDIOBOOK_USB_BRIDGE_PORT,kElfLoader=9021;
constexpr size_t kMaxCatalog=196608,kMaxFile=8ULL*1024*1024*1024;
std::vector<Entry> known;
std::vector<Entry> localKnown;
std::mutex transferMutex;
bool safe(const std::string& s){
 if(s.empty()||s.size()>510||s[0]=='/'||s.back()=='/')return false;
 size_t at=0;
 while(at<s.size()){
  const size_t stop=s.find('/',at),end=stop==std::string::npos?s.size():stop;
  if(end==at||end-at>180||s[at]=='.')return false;
  for(size_t i=at;i<end;i++){
   unsigned char c=(unsigned char)s[i];
   if(c<32||c==127||c=='\\')return false;
  }
  if(stop==std::string::npos)break;
  at=stop+1;
 }
 auto ext=s.find_last_of('.');
 if(ext==std::string::npos)return false;
 std::string suffix=s.substr(ext);
 for(char& c:suffix)if(c>='A'&&c<='Z')c=(char)(c-'A'+'a');
 return suffix==".m4b";
}
bool send_all(int fd,const char *data,size_t len){
 size_t off=0;
 while(off<len){ssize_t n=send(fd,data+off,len-off,0);if(n<=0)return false;off+=(size_t)n;}
 return true;
}
int connect_to(int port){
 int fd=socket(AF_INET,SOCK_STREAM,0);if(fd<0)return -1;
 timeval tv{60,0};setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv));
 setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
 sockaddr_in address{};address.sin_family=AF_INET;
 address.sin_port=htons((uint16_t)port);
 address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
 if(connect(fd,(sockaddr*)&address,sizeof(address))!=0){close(fd);return -1;}
 return fd;
}
bool receive_catalog(int fd,std::string& bytes){
 bytes.clear();char chunk[4096];
 while(bytes.size()<kMaxCatalog){
  ssize_t n=recv(fd,chunk,sizeof(chunk),0);
  if(n<=0)break;
  if((size_t)n>kMaxCatalog-bytes.size())return false;
  bytes.append(chunk,(size_t)n);
  if(bytes.size()>=5 && bytes.compare(bytes.size()-5,5,"DONE\n")==0)break;
 }
 return bytes.find("ABOOK1\n")!=std::string::npos;
}
bool read_data_header(int fd,size_t expected){
 std::string line;char ch=0;
 for(int i=0;i<90;i++){
   if(recv(fd,&ch,1,0)!=1)return false;
   if(ch=='\n')break;
   line+=ch;
 }
 if(ch!='\n')return false;
 unsigned long long length=0;
 return std::sscanf(line.c_str(),"DATA %llu",&length)==1 &&
        length==(unsigned long long)expected;
}
bool local_catalog(std::string& bytes,bool local=false){
 int fd=connect_to(kPort);if(fd<0)return false;
 const char* command=local?"LISTLOCAL\n":"LIST\n";
 bool ok=send_all(fd,command,std::strlen(command));
 if(ok)ok=receive_catalog(fd,bytes);
 close(fd);return ok;
}
bool start_helper(std::string& bytes,std::string& message){
 FILE *elf=fopen("/app0/abookscan.elf","rb");
 if(!elf){
  message="USB helper missing from /app0/abookscan.elf";
  audio_log::write("USB_HELPER_MISSING",message);
  return false;
 }
 int fd=connect_to(kElfLoader);
 if(fd<0){
   fclose(elf);
   message="USB requires elfldr on 127.0.0.1:9021";
   audio_log::write("USB_ELFLDR_ERROR",message);
   return false;
  }
  audio_log::write("USB_HELPER_START","Sending abookscan.elf to elfldr :9021");
 char buffer[8192];bool ok=true;
 while(true){
  size_t n=fread(buffer,1,sizeof(buffer),elf);
  if(!n){if(ferror(elf))ok=false;break;}
  if(!send_all(fd,buffer,n)){ok=false;break;}
 }
 fclose(elf);
 if(ok){shutdown(fd,SHUT_WR);ok=receive_catalog(fd,bytes);}
 close(fd);
 if(!ok){
   message="USB helper did not respond on 9021";
   audio_log::write("USB_HELPER_ERROR",message);
   return false;
  }
 return true;
}
bool parse(const std::string& bytes,Catalog& out){
 size_t p=bytes.find("ABOOK1\n");
 if(p==std::string::npos)return false;
 p+=7;
 while(p<bytes.size()&&out.books.size()<256){
  const size_t end=bytes.find('\n',p);
  if(end==std::string::npos||end-p>120)return false;
  const auto header=bytes.substr(p,end-p);p=end+1;
  if(header=="DONE")return true;
   if(header.rfind("ROOT ",0)==0){
    out.usbRoot=header.substr(5);
    audio_log::write("USB_ROOT",out.usbRoot);
    continue;
   }
  if(header.rfind("ERROR ",0)==0){out.message=header.substr(6);return true;}
  unsigned long long fileSize=0;size_t pathLen=0;
  if(std::sscanf(header.c_str(),"ITEM %zu %llu",&pathLen,&fileSize)!=2||
     pathLen==0||pathLen>510||fileSize==0||fileSize>kMaxFile||
     pathLen>bytes.size()-p)return false;
  const std::string name=bytes.substr(p,pathLen);p+=pathLen;
  if(safe(name))out.books.push_back({name,(size_t)fileSize,out.usbRoot=="/data/AudioBookReader/Books"});
 }
 return false;
}
uint64_t fnv64(const std::string& name){
 uint64_t h=14695981039346656037ULL;
 for(unsigned char c:name){h^=c;h*=1099511628211ULL;}
 return h;
}
bool valid_cached_file(const std::string& path,const std::string& suffix,size_t bytes){
 struct stat st{};
 if(stat(path.c_str(),&st)!=0 || st.st_size<0 ||
    (uint64_t)st.st_size!=bytes || !S_ISREG(st.st_mode))return false;
 FILE* f=fopen(path.c_str(),"rb");if(!f)return false;
 unsigned char sig[12]={};
 const size_t n=fread(sig,1,sizeof(sig),f);fclose(f);
 return suffix==".m4b" && n>=12 && std::memcmp(sig+4,"ftyp",4)==0;

}
bool cachedir(std::string& dir,std::string& why){
 // The PS5 POSIX compatibility layer does not guarantee statvfs support.
 // Calling an unresolved runtime function when opening ANY USB audiobook can
 // crash the console. Only use known-good mkdir/fopen/write primitives.
 // A full disk is reported by fwrite/fflush with errno (ENOSPC), not by a
 // speculative filesystem-capacity query.
 for(const char* path:{AUDIOBOOK_USB_CACHE_ROOT}){
  const std::string base=path;
  const auto slash=base.find_last_of('/');
  if(slash!=std::string::npos)mkdir(base.substr(0,slash).c_str(),0755);
  if(mkdir(path,0755)!=0 && errno!=EEXIST)continue;
  const std::string probe=base+"/.write-probe";
  FILE* out=fopen(probe.c_str(),"wb");
  if(!out)continue;
  const bool writable=fputc('x',out)!=EOF && fflush(out)==0;
  const int code=errno;
  fclose(out);
  remove(probe.c_str());
  if(writable){dir=base;return true;}
  if(code==ENOSPC){why="No free storage for audiobook cache";return false;}
 }
 why="Cannot access a writable Audiobook Reader cache directory";
 return false;
}
} // namespace
bool is_remote(const std::string& path){
 return path.rfind("usbaudio:",0)==0||path.rfind("localaudio:",0)==0;
}
std::string remote_path(const Entry& b){
 return std::string(b.local?"localaudio:":"usbaudio:")+b.relative;
}
bool entry_from_remote(const std::string& path,Entry& out){
 if(!is_remote(path))return false;
 const bool local=path.rfind("localaudio:",0)==0;
 const std::string rel=path.substr(local?11:9);
 const auto& entries=local?localKnown:known;
 for(const auto& b:entries)if(b.relative==rel){out=b;return true;}
 return false;
}
Catalog scan(){
 Catalog out;
 std::string bytes;
 audio_log::write("USB_SCAN_BEGIN","Trying helper :9179 then launching through elfldr :9021");
 if(local_catalog(bytes)){
  out.viaHelper=true;
  audio_log::write("USB_HELPER_REUSE","Connected to ABooks helper on 9179");
 }else if(start_helper(bytes,out.message)){
  out.viaHelper=true;
  audio_log::write("USB_HELPER_READY","ABooks helper returned catalog");
 }else{
  known.clear();
  audio_log::write("USB_SCAN_ERROR",out.message);
  return out;
 }
 if(!parse(bytes,out)){
  out.message="Invalid ABooks helper catalog";
  out.books.clear();out.viaHelper=false;
 }else if(out.message.empty()){
  out.message=std::to_string(out.books.size())+" USB audiobooks via "+
   (out.usbRoot.empty()?"elfldr":out.usbRoot);
 }
 known=out.books;
 audio_log::write(out.viaHelper?"USB_SCAN_DONE":"USB_SCAN_ERROR",
  "USB="+std::to_string(out.books.size())+" "+out.message);
 return out;
}

Catalog scan_local(){
 Catalog out;
 std::string bytes;
 audio_log::write("LOCAL_HELPER_BEGIN","Listing /data/AudioBookReader/Books using ELF helper :9179");
 if(!local_catalog(bytes,true)){
  std::string initial,unused;
  if(!start_helper(unused,initial)||!local_catalog(bytes,true)){
   out.message=initial.empty()?"Local ELF catalog is unavailable":initial;
   audio_log::write("LOCAL_HELPER_ERROR",out.message);
   localKnown.clear();
   return out;
  }
 }
 out.viaHelper=true;
 if(!parse(bytes,out)){
  out.message="Invalid local Books helper catalog";
  out.viaHelper=false;out.books.clear();
 }else if(out.message.empty()){
  for(auto& book:out.books)book.local=true;
  out.message=std::to_string(out.books.size())+" M4B via ELF local Books helper";
 }
 localKnown=out.books;
 audio_log::write("LOCAL_HELPER_DONE","books="+std::to_string(out.books.size())+
   " root="+out.usbRoot+" "+out.message);
 return out;
}
bool fetch_cover(const std::string& remotePath,std::string& encoded,std::string& error){
 encoded.clear();error.clear();
 Entry entry{};
 if(!entry_from_remote(remotePath,entry)){
  error="Book no longer in helper catalog";return false;
 }
 int fd=connect_to(kPort);
 if(fd<0){error="Helper port 9179 unavailable";return false;}
 std::string command=entry.local?"COVERLOCAL ":"COVER ";
 command+=entry.relative+"\n";
 if(command.size()>=600||!send_all(fd,command.data(),command.size())){
  close(fd);error="Could not request companion artwork";return false;
 }
 char ch=0;std::string header;
 while(header.size()<100){
  if(recv(fd,&ch,1,0)!=1)break;
  if(ch=='\n')break;
  header+=ch;
 }
 unsigned long long total=0;
 if(ch!='\n'||std::sscanf(header.c_str(),"DATA %llu",&total)!=1 ||
    total<8||total>6ULL*1024*1024){
  close(fd);error=header.empty()?"No companion cover":header;return false;
 }
 encoded.resize((size_t)total);
 size_t done=0;
 while(done<encoded.size()){
  ssize_t n=recv(fd,&encoded[done],encoded.size()-done,0);
  if(n<=0)break;
  done+=(size_t)n;
 }
 close(fd);
 if(done!=encoded.size()){
  encoded.clear();error="Companion cover transfer interrupted";return false;
 }
 const unsigned char* p=(const unsigned char*)encoded.data();
 if(!((p[0]==0xff&&p[1]==0xd8)||
       (p[0]==0x89&&std::memcmp(p+1,"PNG",3)==0))){
  encoded.clear();error="Unsupported companion image";return false;
 }
 audio_log::write("COVER_COMPANION",remotePath+" image="+std::to_string(done));
 return true;
}
bool cached_copy(const Entry& book,std::string& localPath){
 if(!safe(book.relative)||!book.bytes)return false;
 std::string suffix=book.relative.substr(book.relative.find_last_of('.'));
 for(char& c:suffix)if(c>='A'&&c<='Z')c=(char)(c-'A'+'a');
 char hash[24];std::snprintf(hash,sizeof(hash),"%016llx",
                            (unsigned long long)fnv64(book.relative));
 for(const char* root:{AUDIOBOOK_USB_CACHE_ROOT}){
   std::string path=std::string(root)+(book.local?"/local-":"/usb-")+hash+suffix;
   if(valid_cached_file(path,suffix,book.bytes)){localPath=path;return true;}
 }
 return false;
}
bool fetch(const Entry& book,std::string& localPath,std::string& error,
           ProgressCallback progress,void* context){
 std::lock_guard<std::mutex> lock(transferMutex);
 error.clear();
 audio_log::write("USB_FETCH_START",book.relative+" bytes="+std::to_string(book.bytes));
 if(!safe(book.relative)||book.bytes==0||book.bytes>kMaxFile){error="Invalid USB audiobook";return false;}
 bool found=false;
 const auto& candidates=book.local?localKnown:known;
 for(const auto& b:candidates)
  if(b.relative==book.relative && b.bytes==book.bytes)found=true;
 if(!found){error="USB catalogue expired; press OPTIONS to refresh";return false;}
 std::string suffix=book.relative.substr(book.relative.find_last_of('.'));
 for(char& c:suffix)if(c>='A'&&c<='Z')c=(char)(c-'A'+'a');
 char hex[24];std::snprintf(hex,sizeof(hex),"%016llx",(unsigned long long)fnv64(book.relative));
 const std::string filename=std::string(book.local?"local-":"usb-")+std::string(hex)+suffix;
 // All known cache mounts may contain previously fetched good files.
 // Respect those hits without duplicating them on /data.
 for(const char* base:{AUDIOBOOK_USB_CACHE_ROOT}){
   const std::string prior=std::string(base)+"/"+filename;
   if(valid_cached_file(prior,suffix,book.bytes)){
     localPath=prior;
     audio_log::write("USB_CACHE_HIT",prior);
     if(progress)progress(book.bytes,book.bytes,context);
     return true;
   }
 }
 std::string folder;
 if(!cachedir(folder,error)){
   audio_log::write("USB_CACHE_ERROR",error);return false;
 }
 localPath=folder+"/"+filename;
 const auto cleanup=audio_cache::prune(folder,(uint64_t)book.bytes,localPath);
 if(cleanup.removed)audio_log::write("CACHE_CLEAN",
   "Removed "+std::to_string(cleanup.removed)+" old USB files ("+
   std::to_string(cleanup.bytes/1048576)+" MiB)");
 if(!cleanup.complete)
   audio_log::write("CACHE_SOFT_LIMIT","Not enough evictable entries to meet cache limit");
 // Every file in Cache is a disposable copy. USB originals and Books stay intact.
 if(progress&&!progress(0,book.bytes,context)){
   error="Audiobook loading cancelled";return false;
 }
 int fd=connect_to(kPort);
 if(fd<0){
   auto refreshed=book.local?scan_local():scan();
   if(!refreshed.viaHelper){error=refreshed.message;return false;}
   fd=connect_to(kPort);
   if(fd<0){error="USB audiobook helper no longer responding";return false;}
 }
 const std::string request=std::string(book.local?"FETCHLOCAL ":"FETCH ")+book.relative+"\n";
 if(request.size()>600 || !send_all(fd,request.data(),request.size())){
  close(fd);error="USB fetch request failed";return false;
 }
 const size_t total=book.bytes;
 if(!read_data_header(fd,total)){
  close(fd);error="USB helper rejected book or size changed";return false;
 }
 const std::string tmp=localPath+".part";
 FILE* f=fopen(tmp.c_str(),"wb");
 if(!f){
   const int code=errno;
   close(fd);
   error="Cannot create audiobook cache: "+std::string(std::strerror(code));
   return false;
 }
 size_t written=0;bool ok=true;char buf[32768];
 int ioCode=0;
 const char* reason=nullptr;
 int reconnects=0;
 size_t nextProgress=16*1024*1024;
 while(written<total){
   const size_t needed=std::min(sizeof(buf),total-written);
   ssize_t got=recv(fd,buf,needed,0);
   if(got<0 && errno==EINTR)continue;
   if(got<=0){
     ioCode=got<0?errno:0;
     // File transfers may drop during a long USB read. Ask the helper
     // for the remaining bytes instead of discarding successful data.
     audio_log::write("USB_RETRY",book.relative+" written="+
       std::to_string(written)+"/"+std::to_string(total));
     close(fd);fd=-1;
     if(reconnects++<3){
       fd=connect_to(kPort);
       if(fd<0){
         auto status=book.local?scan_local():scan();
         if(status.viaHelper)fd=connect_to(kPort);
       }
       if(fd>=0){
         const std::string retry=std::string(book.local?"FETCHLOCALR ":"FETCHR ")+
                                 std::to_string(written)+" "+
                                 book.relative+"\n";
         if(retry.size()<=600 && send_all(fd,retry.data(),retry.size()) &&
            read_data_header(fd,total-written)){
           ioCode=0;continue;
         }
         close(fd);fd=-1;
       }
     }
     reason=got==0?"USB connection ended before file completed":
                     "USB read interrupted or timed out";
     ok=false;break;
   }
   if(fwrite(buf,1,(size_t)got,f)!=(size_t)got){
     ioCode=errno;
     reason="Cannot write audiobook to cache";
     ok=false;break;
   }
   written+=(size_t)got;
   if(progress&&!progress(written,total,context)){
     reason="Audiobook loading cancelled";ok=false;break;
   }
   if(written>=nextProgress){
     audio_log::write("USB_PROGRESS",book.relative+" written="+
       std::to_string(written)+"/"+std::to_string(total));
     nextProgress=written+16*1024*1024;
   }
 }
 if(fflush(f)!=0 && ok){
   ioCode=errno;reason="Cache flush failed";ok=false;
 }
 if(fclose(f)!=0 && ok){
   ioCode=errno;reason="Cache close failed";ok=false;
 }
 if(fd>=0)close(fd);
 if(ok && written==total){
   if(rename(tmp.c_str(),localPath.c_str())==0){
     audio_log::write("USB_FETCH_OK",localPath+" bytes="+std::to_string(written));
     return true;
   }
   ioCode=errno;reason="Cannot finalize cache file";ok=false;
 }
 remove(tmp.c_str());
 error=std::string(reason?reason:"USB transfer truncated")+
       " ("+std::to_string(written/1048576)+"/"+
       std::to_string(((size_t)total+1048575)/1048576)+" MiB)";
 if(ioCode)error+=": "+std::string(std::strerror(ioCode));
 audio_log::write("USB_FETCH_ERROR",book.relative+" "+error);
 return false;
}
} // namespace usbaudio
