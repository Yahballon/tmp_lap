// SPDX-License-Identifier: GPL-3.0-or-later
#include "wallpaper_storage.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
int main(){
 using namespace wallpaper_storage;
 const std::string name="MotionBGS-themechanger-storage-ci.mp4";
 assert(folders());
 const std::string live=std::string(kLive)+"/"+name,archive=std::string(kArchive)+"/"+name;
 unlink(live.c_str());unlink(archive.c_str());
 FILE* f=fopen(live.c_str(),"wb");assert(f);
 const char data[]={0,0,0,32,'f','t','y','p','i','s','o','m','a','v','c','1'};
 assert(fwrite(data,1,sizeof(data),f)==sizeof(data));
 char buf[8192]={};assert(fwrite(buf,1,sizeof(buf),f)==sizeof(buf));
 fclose(f);
 assert(valid_file(live));
 assert(preserve(name));
 assert(valid_file(archive));
 unlink(live.c_str());
 assert(!valid_file(live));
 auto r=recover();
 assert(r.restored>=1);
 assert(valid_file(live));
 assert(valid_file(archive));
 assert(preserve(name));
 // Successful re-encodes must replace an older archive, or every reboot
 // restores the old 8-reference stream and requires another conversion.
 const std::string newVersion=live+".new";
 f=fopen(newVersion.c_str(),"wb");assert(f);
 assert(fwrite(data,1,sizeof(data),f)==sizeof(data));
 assert(fwrite(buf,1,sizeof(buf),f)==sizeof(buf));
 assert(fwrite(buf,1,sizeof(buf),f)==sizeof(buf));
 fclose(f);
 assert(rename(newVersion.c_str(),live.c_str())==0);
 assert(preserve(name,true));
 struct stat newInfo{},archivedInfo{};
 assert(stat(live.c_str(),&newInfo)==0);
 assert(stat(archive.c_str(),&archivedInfo)==0);
 assert(newInfo.st_size==archivedInfo.st_size);
 unlink(live.c_str());
 r=recover();
 assert(r.restored>=1);
 assert(stat(live.c_str(),&newInfo)==0);
 assert(newInfo.st_size==archivedInfo.st_size);
 r=recover();
 assert(r.restored==0);
 assert(!filename_ok("../bad"));
 assert(!filename_ok("MotionBGS-../../evil.mp4"));
 unlink(live.c_str());unlink(archive.c_str());
 printf("Persistence test OK: backup, restart restore, idempotence, name checks\n");
}
