// SPDX-License-Identifier: GPL-3.0-or-later
// Bible Reader persistent diagnostic log. No network, no sensitive text.
#pragma once
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <string>
#include <sys/stat.h>
namespace bible_log {
inline constexpr const char* path="/data/BibleReader/bible-reader.log";
inline void write(const char* event,const char* fmt,...){
 static std::mutex guard;
 std::lock_guard<std::mutex> locked(guard);
 (void)mkdir("/data/BibleReader",0755);
 struct stat st{};
 if(stat(path,&st)==0&&st.st_size>1024*1024){
  // Retain one prior diagnostic log, bounded to 1 MiB.
  std::remove("/data/BibleReader/bible-reader.previous.log");
  std::rename(path,"/data/BibleReader/bible-reader.previous.log");
 }
 FILE* out=std::fopen(path,"ab");
 if(!out)return;
 std::time_t now=std::time(nullptr);
 std::fprintf(out,"[%lld] %-16s ",(long long)now,event?event:"EVENT");
 va_list args;va_start(args,fmt);
 std::vfprintf(out,fmt,args);
 va_end(args);
 std::fputc('\n',out);
 std::fflush(out);
 std::fclose(out);
}
inline long fileSize(const std::string& p){
 struct stat st{};
 if(stat(p.c_str(),&st)!=0)return -1;
 if(S_ISDIR(st.st_mode))return -2;
 if(!S_ISREG(st.st_mode))return -3;
 return (long)st.st_size;
}
}
