// SPDX-License-Identifier: GPL-3.0-or-later
// LauncherX-style USB bridge: dedicated read-only ELF helper via elfldr :9021.
// Local library files are never sent to the USB helper.
#include "usb_bridge.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstdint>
#include <mutex>
#include <string>

namespace usbbooks {
namespace {
#ifndef EBOOK_USB_BRIDGE_PORT
#define EBOOK_USB_BRIDGE_PORT 9176
#endif
#ifndef EBOOK_USB_CACHE_ROOT
#define EBOOK_USB_CACHE_ROOT "/download0/ebook-reader-cache"
#endif
constexpr int kPort=EBOOK_USB_BRIDGE_PORT,kElfLoader=9021;
constexpr size_t kMaxCatalog=196608,kMaxFile=2ULL*1024*1024*1024;
std::vector<Entry> known;
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
 return suffix==".epub"||suffix==".pdf"||suffix==".cbr"||suffix==".cbz";
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
 return bytes.find("EBOOK1\n")!=std::string::npos;
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
bool local_catalog(std::string& bytes){
 int fd=connect_to(kPort);if(fd<0)return false;
 bool ok=send_all(fd,"LIST\n",5);
 if(ok)ok=receive_catalog(fd,bytes);
 close(fd);return ok;
}
bool start_helper(std::string& bytes,std::string& message){
 FILE *elf=fopen("/app0/bookscan.elf","rb");
 if(!elf){message="USB helper not installed: bookscan.elf";return false;}
 int fd=connect_to(kElfLoader);
 if(fd<0){fclose(elf);message="USB requires active elfldr on 127.0.0.1:9021";return false;}
 char buffer[8192];bool ok=true;
 while(true){
  size_t n=fread(buffer,1,sizeof(buffer),elf);
  if(!n){if(ferror(elf))ok=false;break;}
  if(!send_all(fd,buffer,n)){ok=false;break;}
 }
 fclose(elf);
 if(ok){shutdown(fd,SHUT_WR);ok=receive_catalog(fd,bytes);}
 close(fd);
 if(!ok){message="USB helper ELF did not respond on port 9021";return false;}
 return true;
}
bool parse(const std::string& bytes,Catalog& out){
 size_t p=bytes.find("EBOOK1\n");
 if(p==std::string::npos)return false;
 p+=7;
 while(p<bytes.size()&&out.books.size()<256){
  const size_t end=bytes.find('\n',p);
  if(end==std::string::npos||end-p>120)return false;
  const auto header=bytes.substr(p,end-p);p=end+1;
  if(header=="DONE")return true;
  if(header.rfind("ERROR ",0)==0){out.message=header.substr(6);return true;}
  unsigned long long fileSize=0;size_t pathLen=0;
  if(std::sscanf(header.c_str(),"ITEM %zu %llu",&pathLen,&fileSize)!=2||
     pathLen==0||pathLen>510||fileSize==0||fileSize>kMaxFile||
     pathLen>bytes.size()-p)return false;
  const std::string name=bytes.substr(p,pathLen);p+=pathLen;
  if(safe(name))out.books.push_back({name,(size_t)fileSize});
 }
 return false;
}
uint64_t fnv64(const std::string& name){
 uint64_t h=14695981039346656037ULL;
 for(unsigned char c:name){h^=c;h*=1099511628211ULL;}
 return h;
}
bool cachedir(size_t needed,std::string& dir,std::string& why){
 // Comics are larger than ebooks; a writable mount can still be full.
 size_t biggest=0;
 bool writeable=false;

 for(const char* path:{EBOOK_USB_CACHE_ROOT,"/data/ebooks/.reader-cache"}){
  const std::string base=path;
  const auto slash=base.find_last_of('/');
  if(slash!=std::string::npos)mkdir(base.substr(0,slash).c_str(),0755);
  if(mkdir(path,0755)!=0&&errno!=EEXIST)continue;
  const auto test=base+"/.write-probe";
  FILE* f=fopen(test.c_str(),"wb");
  if(f){
   fclose(f);remove(test.c_str());writeable=true;
   struct statvfs fs{};
   if(statvfs(base.c_str(),&fs)!=0){dir=base;return true;}
   const unsigned long long freeBytes=(unsigned long long)fs.f_bavail*fs.f_frsize;
   biggest=std::max(biggest,(size_t)std::min(freeBytes,(unsigned long long)SIZE_MAX));
   if(freeBytes >= (unsigned long long)needed + 8ULL*1024*1024){
     dir=base;return true;
   }
  }
 }
 if(!writeable)why="No writable Ebook Reader cache directory";
 else why="Insufficient cache storage: need "+
       std::to_string((needed+1048575)/1048576)+" MiB, free "+
       std::to_string(biggest/1048576)+" MiB";
 return false;
}
} // namespace
bool is_remote(const std::string& path){return path.rfind("usbbooks:",0)==0;}
std::string remote_path(const Entry& b){return "usbbooks:"+b.relative;}
bool entry_from_remote(const std::string& path,Entry& out){
 if(!is_remote(path))return false;
 const std::string rel=path.substr(9);
 for(const auto& b:known)if(b.relative==rel){out=b;return true;}
 return false;
}
Catalog scan(){
 Catalog out;
 std::string bytes;
 if(local_catalog(bytes)){out.viaHelper=true;}
 else if(start_helper(bytes,out.message)){out.viaHelper=true;}
 else return out;
 if(!parse(bytes,out)){out.message="Invalid USB helper catalog";out.books.clear();out.viaHelper=false;}
 else if(out.message.empty())out.message=std::to_string(out.books.size())+" USB books available via elfldr";
 known=out.books;
 return out;
}
bool fetch(const Entry& book,std::string& localPath,std::string& error){
 std::lock_guard<std::mutex> lock(transferMutex);
 error.clear();
 if(!safe(book.relative)||book.bytes==0||book.bytes>kMaxFile){error="Invalid USB book";return false;}
 bool found=false;for(const auto& b:known)if(b.relative==book.relative && b.bytes==book.bytes)found=true;
 if(!found){error="USB catalogue expired; press OPTIONS to refresh";return false;}
 std::string folder;if(!cachedir(book.bytes,folder,error))return false;
 std::string suffix=book.relative.substr(book.relative.find_last_of('.'));
 for(char& c:suffix)if(c>='A'&&c<='Z')c=(char)(c-'A'+'a');
 char hex[24];std::snprintf(hex,sizeof(hex),"%016llx",(unsigned long long)fnv64(book.relative));
 localPath=folder+"/usb-"+hex+suffix;
 // Reuse previously transferred books. Checking the file magic avoids
 // accepting unrelated or partially copied cache files.
 struct stat cached{};
 if(stat(localPath.c_str(),&cached)==0 &&
    cached.st_size>=0 && (size_t)cached.st_size==book.bytes){
   FILE* check=fopen(localPath.c_str(),"rb");
   if(check){
     unsigned char signature[4]={0};
     bool valid=fread(signature,1,4,check)==4;
     fclose(check);
     if(valid && ((suffix==".epub" && signature[0]=='P' && signature[1]=='K') ||
                  (suffix==".pdf" && signature[0]=='%' && signature[1]=='P' &&
                   signature[2]=='D' && signature[3]=='F') ||
                  (suffix==".cbr" && signature[0]=='R' && signature[1]=='a' &&
                   signature[2]=='r' && signature[3]=='!') ||
                  (suffix==".cbz" && signature[0]=='P' && signature[1]=='K')))return true;
   }
 }
 // Local cache is title-specific and never touches original USB content.
 int fd=connect_to(kPort);
 if(fd<0){
   auto refreshed=scan();
   if(!refreshed.viaHelper){error=refreshed.message;return false;}
   fd=connect_to(kPort);
   if(fd<0){error="USB book helper no longer responding";return false;}
 }
 const std::string request="FETCH "+book.relative+"\n";
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
   error="Cannot create comic cache: "+std::string(std::strerror(code));
   return false;
 }
 size_t written=0;bool ok=true;char buf[32768];
 int ioCode=0;
 const char* reason=nullptr;
 while(written<(size_t)total){
   const size_t needed=std::min(sizeof(buf),(size_t)total-written);
   ssize_t got=recv(fd,buf,needed,0);
   if(got<0 && errno==EINTR)continue;
   if(got<=0){
     ioCode=got<0?errno:0;
     reason=got==0?"USB connection closed early":"USB socket read timed out or failed";
     ok=false;break;
   }
   if(fwrite(buf,1,(size_t)got,f)!=(size_t)got){
     ioCode=errno;
     reason="Cannot write comic to cache";
     ok=false;break;
   }
   written+=(size_t)got;
 }
 if(fflush(f)!=0 && ok){
   ioCode=errno;reason="Cache flush failed";ok=false;
 }
 if(fclose(f)!=0 && ok){
   ioCode=errno;reason="Cache close failed";ok=false;
 }
 close(fd);
 if(ok && written==(size_t)total){
   if(rename(tmp.c_str(),localPath.c_str())==0)return true;
   ioCode=errno;reason="Cannot finalize cache file";ok=false;
 }
 remove(tmp.c_str());
 error=std::string(reason?reason:"USB transfer truncated")+
       " ("+std::to_string(written/1048576)+"/"+
       std::to_string(((size_t)total+1048575)/1048576)+" MiB)";
 if(ioCode)error+=": "+std::string(std::strerror(ioCode));
 return false;
}
} // namespace usbbooks
