// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// PS5-safe file diagnostics. Uses the same mkdir/fopen/fwrite primitives
// already supported by the Audiobook Reader; no statvfs or external logger.
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <sys/stat.h>

#ifndef AUDIOBOOK_DATA_ROOT
#define AUDIOBOOK_DATA_ROOT "/data/AudioBookReader"
#endif
namespace audio_log {
inline std::string root() {return AUDIOBOOK_DATA_ROOT;}
inline std::string books_directory(){return root()+"/Books";}
inline std::string filename(){return root()+"/audiobook-reader.log";}
inline std::mutex& mutex(){static std::mutex value;return value;}
inline bool prepare(){
 const auto dir=root();
 if(mkdir(dir.c_str(),0755)!=0 && errno!=EEXIST)return false;
 const auto books=books_directory();
 // A read-only/missing Books folder must not suppress diagnostics.
 mkdir(books.c_str(),0755);
 FILE* probe=fopen(filename().c_str(),"ab");
 if(!probe)return false;
 fclose(probe);
 return true;
}
inline void write(const char* event,const std::string& detail){
 std::lock_guard<std::mutex> lock(mutex());
 if(!prepare())return; // Logging must never prevent reader startup.
 const std::string file=filename();
 struct stat current{};
 if(stat(file.c_str(),&current)==0 && current.st_size>2*1024*1024){
   const std::string previous=file+".1";
   remove(previous.c_str());
   rename(file.c_str(),previous.c_str());
 }
 FILE* out=fopen(file.c_str(),"ab");
 if(!out)return;
 std::string clean=detail;
 if(clean.size()>850)clean.resize(850);
 for(char& c:clean)if(c=='\n'||c=='\r'||c=='\t')c=' ';
 std::fprintf(out,"[AudioBookReader] %-18s %s\n",event?event:"EVENT",clean.c_str());
 fclose(out);
}
} // namespace audio_log
