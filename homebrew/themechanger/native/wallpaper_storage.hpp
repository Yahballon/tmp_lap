// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Persistent ThemeChanger-owned archive; LiveShell's video folder is a
// deployment/cache location, never the sole copy of a user's downloads.
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
namespace wallpaper_storage {
static constexpr const char* kArchive="/data/ThemeChanger/wallpapers";
static constexpr const char* kLive="/data/LiveShell/video";
struct Report { int archived=0,restored=0,errors=0,live=0,backups=0; };
inline bool filename_ok(const std::string& name){
 if(name.size()<15||name.size()>125||name.rfind("MotionBGS-",0)!=0 ||
    name.substr(name.size()-4)!=".mp4")return false;
 for(size_t i=10;i<name.size()-4;i++){
  const char c=name[i];
  if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'))return false;
 }
 return true;
}
inline bool valid_file(const std::string& name){
 struct stat st{};
 if(stat(name.c_str(),&st)!=0||!S_ISREG(st.st_mode)||st.st_size<1024)return false;
 FILE* f=fopen(name.c_str(),"rb");if(!f)return false;
 unsigned char h[12]{};
 const bool ok=fread(h,1,12,f)==12&&memcmp(h+4,"ftyp",4)==0;
 fclose(f);return ok;
}
inline bool folders(){
 if(mkdir("/data/ThemeChanger",0755)!=0&&errno!=EEXIST)return false;
 if(mkdir(kArchive,0755)!=0&&errno!=EEXIST)return false;
 if(mkdir("/data/LiveShell",0755)!=0&&errno!=EEXIST)return false;
 return mkdir(kLive,0755)==0||errno==EEXIST;
}
inline bool durable_copy(const std::string& source,const std::string& dst){
 if(!valid_file(source))return false;
 // Never leave incomplete MP4 files that are interpreted as installed.
 const std::string temp=dst+".part";
 unlink(temp.c_str());
 if(link(source.c_str(),temp.c_str())==0){
  if(rename(temp.c_str(),dst.c_str())==0)return true;
  unlink(temp.c_str());return false;
 }
 FILE* in=fopen(source.c_str(),"rb");
 if(!in)return false;
 FILE* out=fopen(temp.c_str(),"wb");
 if(!out){fclose(in);return false;}
 char chunk[65536];size_t n;
 bool ok=true;
 while((n=fread(chunk,1,sizeof(chunk),in))!=0){
  if(fwrite(chunk,1,n,out)!=n){ok=false;break;}
 }
 if(ferror(in))ok=false;
 if(fflush(out)!=0||fsync(fileno(out))!=0)ok=false;
 if(fclose(out)!=0)ok=false;
 fclose(in);
 if(!ok||!valid_file(temp)||rename(temp.c_str(),dst.c_str())!=0){
  unlink(temp.c_str());return false;
 }
 return true;
}
inline bool preserve(const std::string& name,bool replace_existing=false){
 if(!filename_ok(name)||!folders())return false;
 const std::string dst=std::string(kArchive)+"/"+name;
 // Overwrite the backup only after a completed, validated re-encode.
 if(!replace_existing&&valid_file(dst))return true;
 return durable_copy(std::string(kLive)+"/"+name,dst);
}
inline Report recover(){
 Report r{};
 if(!folders()){r.errors++;return r;}
 // First, recover backups into LiveShell for its scanner. This must run
 // BEFORE Catalog::load and LiveShell is started/handover is attempted.
 if(DIR* d=opendir(kArchive)){
  while(auto* e=readdir(d)){
   std::string name=e->d_name;
   if(!filename_ok(name))continue;
   const std::string original=std::string(kArchive)+"/"+name;
   if(!valid_file(original))continue;
   r.backups++;
   if(valid_file(std::string(kLive)+"/"+name))continue;
   if(durable_copy(original,std::string(kLive)+"/"+name))r.restored++;
   else r.errors++;
  }
  closedir(d);
 }else r.errors++;
 // Migrate videos downloaded with ThemeChanger <= v0.3.9 into archive.
 if(DIR* d=opendir(kLive)){
  while(auto* e=readdir(d)){
   std::string name=e->d_name;
   if(!filename_ok(name)||!valid_file(std::string(kLive)+"/"+name))continue;
   r.live++;
   const std::string backup=std::string(kArchive)+"/"+name;
   if(valid_file(backup))continue;
   if(durable_copy(std::string(kLive)+"/"+name,backup)){r.archived++;r.backups++;}
   else r.errors++;
  }
  closedir(d);
 }else r.errors++;
 return r;
}
} // namespace wallpaper_storage
