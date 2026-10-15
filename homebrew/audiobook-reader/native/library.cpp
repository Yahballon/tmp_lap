// SPDX-License-Identifier: GPL-3.0-or-later
#include "library.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <unordered_map>
namespace audio {
#ifndef AUDIOBOOK_DATA_ROOT
#define AUDIOBOOK_DATA_ROOT "/data/AudioBookReader"
#endif
std::string root(){return AUDIOBOOK_DATA_ROOT;}
std::string clean_title(std::string path){
 auto slash=path.find_last_of('/');if(slash!=std::string::npos)path=path.substr(slash+1);
 auto dot=path.find_last_of('.');if(dot!=std::string::npos)path.resize(dot);
 for(char& c:path)if(c=='_'||c=='-')c=' ';
 return path.empty()?"Unknown audiobook":path;
}
namespace {
bool m4b(const std::string& s){if(s.size()<4)return false;
 std::string e=s.substr(s.size()-4);for(char& c:e)if(c>='A'&&c<='Z')c+=32;return e==".m4b";}
void walk(const std::string& path,std::vector<Book>& result,int depth){
 if(depth>3||result.size()>=1024)return;
 DIR* dir=opendir(path.c_str());if(!dir)return;
 while(auto* item=readdir(dir)){
  const std::string name=item->d_name;
  if(name.empty()||name[0]=='.')continue;
  const std::string file=path+"/"+name;
  struct stat st{};if(lstat(file.c_str(),&st)!=0)continue;
  if(S_ISDIR(st.st_mode)){walk(file,result,depth+1);continue;}
  if(S_ISREG(st.st_mode)&&st.st_size>1024&&m4b(name))
    result.push_back({file,clean_title(name),"Unknown author",(int64_t)st.st_size});
  if(result.size()>=1024)break;
 }
 closedir(dir);
}
std::string statefile(){return root()+"/progress.tsv";}
bool is_safe(const std::string& s){return !s.empty()&&s.size()<1024&&s.find('\t')==std::string::npos&&s.find('\n')==std::string::npos;}
}
void scan(const std::vector<std::string>& roots,std::vector<Book>& out){
 out.clear();
 for(auto& dir:roots)walk(dir,out,0);
 std::sort(out.begin(),out.end(),[](const Book& a,const Book& b){return a.title<b.title;});
 out.erase(std::unique(out.begin(),out.end(),[](const Book& a,const Book& b){return a.path==b.path;}),out.end());
 load(out);
}
void load(std::vector<Book>& books){
 FILE* f=fopen(statefile().c_str(),"rb");if(!f)return;
 char line[4096];std::unordered_map<std::string,Book> stored;
 while(fgets(line,sizeof(line),f)){
  char* a=strchr(line,'\t');if(!a)continue;*a++=0;
  char* b=strchr(a,'\t');if(!b)continue;*b++=0;
  char* c=strchr(b,'\t');if(!c)continue;*c++=0;
  char* end=nullptr;double pos=strtod(a,&end);if(end!=b-1||pos<0||pos>1728000)continue;
  double dur=strtod(b,&end);if(end!=c-1||dur<0||dur>1728000)continue;
  Book saved;saved.position=pos;saved.duration=dur;saved.favorite=(*c=='1');
  stored[line]=saved;
 }
 fclose(f);
 for(auto& book:books){auto it=stored.find(book.path);if(it==stored.end())continue;
  book.position=it->second.position;book.duration=it->second.duration;book.favorite=it->second.favorite;}
}
bool save(const std::vector<Book>& books){
 mkdir(root().c_str(),0755);
 std::string temp=statefile()+".tmp";
 FILE* f=fopen(temp.c_str(),"wb");if(!f)return false;
 bool ok=fputs("# AudioBookReader state v1\n",f)>=0;
 for(const auto& b:books)if(is_safe(b.path)&&ok)
  ok=fprintf(f,"%s\t%.3f\t%.3f\t%d\n",b.path.c_str(),b.position,b.duration,b.favorite?1:0)>0;
 if(fflush(f)!=0)ok=false;
 if(fclose(f)!=0)ok=false;
 if(ok&&rename(temp.c_str(),statefile().c_str())==0)return true;
 remove(temp.c_str());return false;
}
}
