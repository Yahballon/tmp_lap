// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Opt-in, explicitly confirmed operations via a one-shot private ELF helper.
// No destructive action happens during inventory discovery or icon fetch.
#include "inventory.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <cstdint>
namespace uninstaller::actions {
inline constexpr const char *kFolder="/data/Uninstaller/actions";
struct Result{bool launched=false,accepted=false,verified=false;int code=-1;std::string detail;};

inline bool eligible(const Item& a){
  if(!a.remote)return isAllowedRemoval(a);
  if(a.kind==Kind::Installed){
    return validTitleId(a.id)&&a.id!="PPSA99986" &&
           a.path=="/user/app/"+a.id;
  }
  if(a.kind!=Kind::UsbPackage || !endsPkg(a.path))return false;
  if(a.path.rfind("/mnt/usb",0)!=0||a.path.size()<18||
     a.path[8]<'0'||a.path[8]>'7'||a.path[9]!='/')return false;
  return a.path.find("..")==std::string::npos &&
         a.path.find('\\')==std::string::npos;
}
inline bool ownedPrivate(const char* path,bool directory){
  struct stat st{};
  if(lstat(path,&st)!=0||st.st_uid!=getuid()||(st.st_mode&0077))return false;
  return directory?S_ISDIR(st.st_mode):S_ISREG(st.st_mode)&&st.st_nlink==1;
}
inline bool writeRequest(const Item& item,std::string& token,std::string& why){
  token.clear();
  if(!eligible(item)){why="Unsafe or unsupported source";return false;}
  if(mkdir(kFolder,0700)!=0&&errno!=EEXIST){
    why="Cannot create private actions folder";return false;
  }
  if(!ownedPrivate(kFolder,true)){why="Action folder is not private; aborting";return false;}
  // Require OS cryptographic randomness; no timestamp or predictable nonce fallback.
  unsigned char entropy[16]={0};
  int rd=open("/dev/urandom",O_RDONLY);
  if(rd<0 || read(rd,entropy,sizeof(entropy))!=(ssize_t)sizeof(entropy)){
    if(rd>=0)close(rd);
    why="Secure random source unavailable";return false;
  }
  close(rd);
  char n[33];
  for(int i=0;i<16;i++)std::snprintf(n+i*2,3,"%02x",(unsigned)entropy[i]);
  token=n;
  const std::string content=std::string("nonce=")+token+"\n"+
       (item.remote&&item.kind==Kind::Installed
         ? "kind=TITLE\ntitle="+item.id+"\n"
         : "kind=USBPKG\npath="+item.path+"\n");
  // Clear old result and request only if they are regular files owned by us.
  for(const char* name:{"/data/Uninstaller/actions/result",
                         "/data/Uninstaller/actions/request"}){
    struct stat st{};
    if(lstat(name,&st)==0){
      if(!ownedPrivate(name,false)){why="Unsafe action state file";return false;}
      if(unlink(name)!=0){why="Cannot reset action state";return false;}
    }else if(errno!=ENOENT){why="Cannot inspect action state";return false;}
  }
  int fd=open("/data/Uninstaller/actions/request",
              O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
  if(fd<0){why="Cannot write private request";return false;}
  const bool ok=write(fd,content.data(),content.size())==(ssize_t)content.size()
       &&fsync(fd)==0;
  close(fd);
  if(!ok){why="Request write failed";return false;}
  return true;
}
inline Result execute(const Item& item,ScanLog logger=nullptr,
                      const char* executable="/app0/unremove.elf"){
  Result result;
  if(!eligible(item)){result.detail="Title cannot be safely removed from this source";return result;}
  std::string nonce;
  if(!writeRequest(item,nonce,result.detail)){
    if(logger)logger("REMOVE_PRECHECK",result.detail.c_str());
    return result;
  }
  FILE* file=fopen(executable,"rb");
  if(!file){result.detail="unremove.elf missing";return result;}
  int fd=socket(AF_INET,SOCK_STREAM,0);
  if(fd<0){fclose(file);result.detail="Loader socket unavailable";return result;}
  struct timeval tv{12,0};
  setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
  setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv));
  sockaddr_in addr{};addr.sin_family=AF_INET;
  addr.sin_port=htons(9021);addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  if(connect(fd,(sockaddr*)&addr,sizeof(addr))!=0){
    result.detail="elfldr not running on port 9021";
    fclose(file);close(fd);return result;
  }
  result.launched=true;
  if(logger)logger("REMOVE_DISPATCH",item.kind==Kind::Installed?item.id.c_str():"USBPKG");
  char chunk[8192];bool transmitted=true;
  for(;;){
    size_t n=fread(chunk,1,sizeof(chunk),file);
    if(!n){if(ferror(file))transmitted=false;break;}
    size_t offset=0;
    while(offset<n){
      ssize_t sent=send(fd,chunk+offset,n-offset,0);
      if(sent<=0){transmitted=false;break;}
      offset+=(size_t)sent;
    }
    if(!transmitted)break;
  }
  fclose(file);
  if(transmitted)shutdown(fd,SHUT_WR);
  std::string reply;
  while(reply.size()<4096){
    ssize_t got=recv(fd,chunk,sizeof(chunk),0);
    if(got<=0)break;
    reply.append(chunk,(size_t)got);
  }
  close(fd);
  if(!transmitted){result.detail="Unable to deliver ELF to loader";return result;}
  // The payload writes an authenticated response into the private directory.
  FILE* answer=fopen("/data/Uninstaller/actions/result","rb");
  if(!answer){result.detail="No removal result; check system state";return result;}
  char buf[512]={0};size_t len=fread(buf,1,sizeof(buf)-1,answer);fclose(answer);
  std::string report(buf,len);
  const std::string expected="nonce="+nonce+"\n";
  if(report.rfind(expected,0)!=0){
    result.detail="Action response nonce mismatch";return result;
  }
  unsigned code=0,state=0;
  const size_t c=report.find("code="),s=report.find("state=");
  if(c==std::string::npos||s==std::string::npos||
     std::sscanf(report.c_str()+c,"code=%x",&code)!=1||
     std::sscanf(report.c_str()+s,"state=%u",&state)!=1){
    result.detail="Malformed action response";return result;
  }
  result.code=(int)code;
  result.accepted=(code==0 && state!=0);
  result.verified=(code==0 && state==2);
  result.detail=code!=0?"Native removal rejected (0x"+[] (unsigned n){
      char h[20];std::snprintf(h,sizeof(h),"%08X",n);return std::string(h);
    }(code)+")"
    : result.verified?"USB PKG removed; installed game untouched":
      "Native title uninstall requested; refresh to verify completion";
  if(logger)logger("REMOVE_RESULT",result.detail.c_str());
  // Do not report a Sony async acceptance as verified deletion.
  return result;
}
} // namespace uninstaller::actions
