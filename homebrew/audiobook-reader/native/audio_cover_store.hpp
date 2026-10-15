// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Persistent small JPEG/PNG/GIF thumbnail cache for USB CBR/CBZ/EPUB.
// Separate from disposable full archives; does not touch user's books.
#include "audio_log.hpp"
#include <cstdint>
#include <cstdio>
#include <string>
#include <mutex>
#include <sys/stat.h>
#include <unistd.h>
namespace audio_cover {
inline std::string directory(){return audio_log::root()+"/Covers";}
inline std::string path(const std::string& key){
 uint64_t h=14695981039346656037ULL;
 for(unsigned char c:key){h^=c;h*=1099511628211ULL;}
 char name[40];std::snprintf(name,sizeof(name),"cover-%016llx.img",(unsigned long long)h);
 return directory()+"/"+name;
}
inline bool image(const std::string& bytes){
 if(bytes.size()<8||bytes.size()>6*1024*1024)return false;
 const unsigned char* p=(const unsigned char*)bytes.data();
 return (p[0]==0xff&&p[1]==0xd8) ||
        (p[0]==0x89&&bytes.compare(1,3,"PNG")==0) ||
        bytes.compare(0,3,"GIF")==0;
}
inline bool load(const std::string& key,std::string& output){
 output.clear();
 FILE* f=std::fopen(path(key).c_str(),"rb");if(!f)return false;
 if(std::fseek(f,0,SEEK_END)!=0){fclose(f);return false;}
 long n=ftell(f);
 if(n<8||n>6*1024*1024){fclose(f);return false;}
 rewind(f);output.resize((size_t)n);
 const bool ok=std::fread(&output[0],1,output.size(),f)==output.size();
 fclose(f);
 if(!ok||!image(output)){output.clear();return false;}
 return true;
}
inline bool save(const std::string& key,const std::string& encoded){
 static std::mutex saveMutex;
 std::lock_guard<std::mutex> lock(saveMutex);
 if(!image(encoded)||!audio_log::prepare())return false;
 const std::string folder=directory();
 if(mkdir(folder.c_str(),0755)!=0&&errno!=EEXIST)return false;
 const std::string filename=path(key),tmp=filename+".part";
 FILE* f=fopen(tmp.c_str(),"wb");if(!f)return false;
 bool ok=fwrite(encoded.data(),1,encoded.size(),f)==encoded.size();
 if(fflush(f)!=0)ok=false;
 if(fclose(f)!=0)ok=false;
 if(!ok){remove(tmp.c_str());return false;}
 if(rename(tmp.c_str(),filename.c_str())!=0){remove(tmp.c_str());return false;}
 return true;
}
} // namespace audio_cover
