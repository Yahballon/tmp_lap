#include "../native/library.hpp"
#include "../native/audio_log.hpp"
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <algorithm>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
int main(){
 const std::string dir=audio::root();
 assert(audio_log::prepare());
 audio_log::write("TEST","Logging to /data/AudioBookReader");
 assert(audio_log::filename()==dir+"/audiobook-reader.log");
 struct stat logFile{};
 assert(stat(audio_log::filename().c_str(),&logFile)==0 && logFile.st_size>0);
 mkdir(dir.c_str(),0755);mkdir((dir+"/Books").c_str(),0755);
 const std::string file=dir+"/Books/test.M4B";
 const std::string nested=dir+"/Books/Series";
 assert(mkdir(nested.c_str(),0755)==0 || errno==EEXIST);
 const std::string child=nested+"/chapter.m4b";
 FILE* n=fopen(child.c_str(),"wb");assert(n);
 for(int i=0;i<4096;i++)fputc('x',n);
 assert(fclose(n)==0);
 const std::string loose=dir+"/loose.m4b";
 FILE* p=fopen(loose.c_str(),"wb");assert(p);
 for(int i=0;i<2048;i++)fputc('x',p);
 assert(fclose(p)==0);
 FILE* f=fopen(file.c_str(),"wb");assert(f);
 for(int i=0;i<2048;i++)fputc('x',f);
 fclose(f);
 std::vector<audio::Book> found;audio::scan({dir+"/Books"},found);
 assert(found.size()==2);
 assert(audio::scan_report().find("DIR="+dir+"/Books")!=std::string::npos);
 assert(audio::scan_report().find("m4b=2")!=std::string::npos);
 for(auto& b:found)if(b.title=="chapter")assert(b.path==child);
 std::vector<audio::Book> all;
 audio::scan({dir+"/Books",dir},all);
 assert(all.size()==3);
 // The state root only scans direct M4B files; Cache is never imported.
 assert(audio::scan_report().find("DIR="+dir+" opened=1")!=std::string::npos);
 auto iter=std::find_if(found.begin(),found.end(),[](const auto& b){return b.title=="test";});
 assert(iter!=found.end());
 iter->position=135.2;iter->duration=3600;iter->favorite=true;
 assert(audio::save(found));iter->position=0;iter->favorite=false;audio::load(found);
 assert(iter->position>135&&iter->favorite);
 remove(file.c_str());remove(child.c_str());remove(loose.c_str());rmdir(nested.c_str());
 puts("Audiobook library smoke test passed");
}
