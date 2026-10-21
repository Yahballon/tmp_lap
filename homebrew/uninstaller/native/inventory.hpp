// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <vector>
namespace uninstaller {
enum class Kind { Installed, Homebrew, UsbPackage };
struct Item {
  std::string name, id, version, path, icon, root;
  Kind kind = Kind::Homebrew;
  std::uint64_t size=0;
  bool sizeKnown=false;
};
struct Storage {std::uint64_t total=0,free=0;bool available=false;};
inline bool validTitleId(const std::string& id) {
  if(id.size()!=9)return false;
  if(id.compare(0,4,"PPSA") && id.compare(0,4,"CUSA"))return false;
  for(size_t i=4;i<9;i++)if(id[i]<'0'||id[i]>'9')return false;
  return true;
}
inline bool normalName(const std::string& name) {
  return !name.empty()&&name!="."&&name!=".."&&name.find('/')==std::string::npos
      &&name.find('\\')==std::string::npos&&name.size()<230;
}
inline bool regular(const std::string& path) {
  struct stat s{};return lstat(path.c_str(),&s)==0 && S_ISREG(s.st_mode);
}
inline bool directory(const std::string& path) {
  struct stat s{};return lstat(path.c_str(),&s)==0 && S_ISDIR(s.st_mode);
}
inline std::string readSmall(const std::string& path) {
  FILE* f=std::fopen(path.c_str(),"rb");if(!f)return {};
  char b[8193];size_t n=std::fread(b,1,8192,f);std::fclose(f);b[n]='\0';return std::string(b,n);
}
inline std::string field(const std::string& data,const char* key) {
  const std::string k=std::string("\"")+key+"\"";
  size_t at=data.find(k);if(at==std::string::npos)return {};
  at=data.find(':',at+k.size());if(at==std::string::npos)return {};
  at=data.find('"',at+1);if(at==std::string::npos)return {};
  std::string out;bool escape=false;
  for(size_t p=at+1;p<data.size()&&out.size()<95;p++){
    const unsigned char c=(unsigned char)data[p];
    if(!escape&&c=='"')break;
    if(!escape&&c=='\\'){escape=true;continue;}
    if(c>=32)out+=(char)c;
    escape=false;
  }
  return out;
}
inline bool endsPkg(const std::string& n) {
  return n.size()>4 && (n.substr(n.size()-4)==".pkg"||n.substr(n.size()-4)==".PKG");
}
inline void addFolders(std::vector<Item>& out,const std::string& root,Kind kind) {
  DIR* d=opendir(root.c_str());if(!d)return;
  struct dirent* e;
  while((e=readdir(d)) && out.size()<750){
    const std::string id=e->d_name;
    if(!validTitleId(id))continue;
    const std::string path=root+"/"+id;
    if(!directory(path))continue;
    const std::string param=path+"/sce_sys/param.json";
    const bool hasExecutable=regular(path+"/eboot.bin");
    if(!hasExecutable && !regular(param))continue;
    const std::string info=readSmall(param);
    Item a;a.id=id;a.path=path;a.root=root;a.kind=kind;
    a.name=field(info,"titleName");if(a.name.empty())a.name=id;
    a.version=field(info,"contentVersion");
    if(regular(path+"/sce_sys/icon0.png"))a.icon=path+"/sce_sys/icon0.png";
    out.push_back(std::move(a));
  }
  closedir(d);
}
inline void scanPkgs(std::vector<Item>& out,const std::string& root,unsigned depth=0) {
  if(depth>2||out.size()>=750)return;
  DIR* d=opendir(root.c_str());if(!d)return;
  struct dirent* e;
  while((e=readdir(d)) && out.size()<750){
    std::string name=e->d_name;
    if(!normalName(name)||name[0]=='.')continue;
    const std::string path=root+"/"+name;
    struct stat st{};if(lstat(path.c_str(),&st)!=0)continue;
    if(S_ISLNK(st.st_mode))continue;
    if(S_ISDIR(st.st_mode)){
      if(depth<2 && name!="homebrew" && name!="system_data" && name!="user")
        scanPkgs(out,path,depth+1);
      continue;
    }
    if(!S_ISREG(st.st_mode)||!endsPkg(name))continue;
    Item a;a.name=name;a.path=path;a.root=root;a.kind=Kind::UsbPackage;
    a.size=(std::uint64_t)st.st_size;a.sizeKnown=true;out.push_back(std::move(a));
  }
  closedir(d);
}
inline std::vector<Item> scan() {
  std::vector<Item> out;
  addFolders(out,"/data/homebrew",Kind::Homebrew);
  addFolders(out,"/user/app",Kind::Installed);
  addFolders(out,"/system_ex/app",Kind::Installed);
  for(int i=0;i<4;i++){
    const std::string usb="/mnt/usb"+std::to_string(i);
    addFolders(out,usb+"/homebrew",Kind::Homebrew);
    scanPkgs(out,usb);
  }
  // Favor separately managed installed applications over any duplicate app registrations.
  std::sort(out.begin(),out.end(),[](const Item& a,const Item& b){
    if(a.kind!=b.kind)return (int)a.kind<(int)b.kind;
    return a.name<b.name;
  });
  out.erase(std::unique(out.begin(),out.end(),[](const Item& a,const Item& b){
    return a.path==b.path;
  }),out.end());
  return out;
}
inline Storage storage(const std::string& path="/data") {
  Storage s;struct statvfs v{};
  if(statvfs(path.c_str(),&v)==0&&v.f_frsize&&v.f_blocks){
    s.total=(std::uint64_t)v.f_blocks*v.f_frsize;
    s.free=(std::uint64_t)v.f_bavail*v.f_frsize;
    s.available=s.free<=s.total;
  }
  return s;
}
inline std::string bytes(std::uint64_t n) {
  double v=(double)n;int i=0;
  static const char* units[]={"B","KB","MB","GB","TB"};
  while(v>=1024&&i<4){v/=1024;++i;}
  char b[40];std::snprintf(b,sizeof(b),"%.1f %s",v,units[i]);
  return b;
}
// No symlink, mount or path traversal is ever followed when removing user content.
inline bool isAllowedRemoval(const Item& a) {
  if(a.kind==Kind::Installed)return false; // must use AppInst API, not unlink().
  if(a.kind==Kind::UsbPackage) {
    if(a.path.rfind("/mnt/usb",0)!=0||a.path.size()<15)return false;
    return endsPkg(a.path)&&normalName(a.path.substr(a.path.rfind('/')+1))
       &&regular(a.path);
  }
  if(!validTitleId(a.id))return false;
  if(a.path=="/data/homebrew/"+a.id && a.root=="/data/homebrew")return directory(a.path);
  for(int i=0;i<4;i++){
    const std::string parent="/mnt/usb"+std::to_string(i)+"/homebrew";
    if(a.root==parent && a.path==parent+"/"+a.id)return directory(a.path);
  }
  return false;
}
inline bool clearTree(const std::string& path,unsigned level=0) {
  if(level>20)return false;
  struct stat st{};if(lstat(path.c_str(),&st)!=0)return false;
  if(!S_ISDIR(st.st_mode))return unlink(path.c_str())==0;
  DIR* d=opendir(path.c_str());if(!d)return false;
  bool ok=true;struct dirent* e;
  while((e=readdir(d))){
    const std::string child=e->d_name;if(child=="."||child=="..")continue;
    if(!normalName(child)||!clearTree(path+"/"+child,level+1)){ok=false;break;}
  }
  closedir(d);
  if(ok)ok=rmdir(path.c_str())==0;
  return ok;
}
inline int removeFiles(const Item& a) {
  if(!isAllowedRemoval(a))return EPERM;
  if(a.kind==Kind::UsbPackage)return unlink(a.path.c_str())==0?0:errno;
  return clearTree(a.path)?0:(errno?errno:EIO);
}
inline std::uint64_t shallowSize(const Item& a) {
  if(a.sizeKnown)return a.size;
  // Folder scans may contain many GB; don't block the renderer or invent totals.
  return 0;
}
}