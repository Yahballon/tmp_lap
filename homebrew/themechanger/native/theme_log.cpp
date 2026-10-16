#include "theme_log.hpp"
#include <cstdio>
#include <mutex>
#include <ctime>
#include <sys/stat.h>
#include <cerrno>
namespace tc {
void log(const char* event,const char* detail) {
 static std::mutex mutex;
 std::lock_guard<std::mutex> lock(mutex);
 if(mkdir("/data/ThemeChanger",0755)!=0 && errno!=EEXIST)return;
 const char* path="/data/ThemeChanger/themechanger.log";
 struct stat info{};
 if(stat(path,&info)==0 && info.st_size>2*1024*1024) rename(path,"/data/ThemeChanger/themechanger.old.log");
 FILE* f=fopen(path,"ab");if(!f)return;
 fprintf(f,"[%lld] %-22s %s\n",(long long)time(nullptr),event?event:"EVENT",detail?detail:"");
 fflush(f);fclose(f);
}
}
