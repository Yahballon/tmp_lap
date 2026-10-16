// SPDX-License-Identifier: GPL-3.0-or-later
// Native catalog and background workers for user's personal MotionBGS downloads.
#include "motion_catalog.hpp"
#include "motion_download.hpp"
#include "liveshell_client.hpp"
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdio>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
namespace motion {
namespace {
bool valid_slug(const std::string& slug) {
 if(slug.size()<3||slug.size()>100)return false;
 for(unsigned char c:slug)if(!(std::isalnum(c)||c=='-'))return false;
 return true;
}
bool make_directory(const char* p){return mkdir(p,0755)==0||errno==EEXIST;}
bool exists(const std::string& p,long minimum=500){
 struct stat st{};return stat(p.c_str(),&st)==0&&st.st_size>minimum;
}
bool is_mp4(const std::string& path){
 FILE* f=fopen(path.c_str(),"rb");if(!f)return false;
 unsigned char b[12]{};bool ok=fread(b,1,sizeof(b),f)==sizeof(b);
 fclose(f);return ok&&b[4]=='f'&&b[5]=='t'&&b[6]=='y'&&b[7]=='p';
}
bool is_image(const std::string& path){
 FILE* f=fopen(path.c_str(),"rb");if(!f)return false;
 unsigned char b[12]{};size_t n=fread(b,1,sizeof(b),f);fclose(f);
 return (n>=4&&b[0]==0xff&&b[1]==0xd8)||(n>=8&&b[0]==137&&b[1]=='P'&&b[2]=='N'&&b[3]=='G');
}
std::string clean_label(std::string s){
 for(auto& c:s)if(c=='\r'||c=='\n'||c=='\t')c=' ';
 return s;
}
}
bool Catalog::load(const std::string& assets){
 std::string path=assets+"/motionbgs/catalog.tsv";
 FILE* f=fopen(path.c_str(),"rb");if(!f){report("MotionBGS catalog missing");return false;}
 char line[2048];std::vector<Entry> next;
 while(fgets(line,sizeof(line),f)&&next.size()<200){
  std::string s(line);if(s.empty()||s[0]=='#')continue;
  if(!s.empty()&&s.back()=='\n')s.pop_back();
  size_t first=s.find('\t'),second=first==std::string::npos?first:s.find('\t',first+1),
   third=second==std::string::npos?second:s.find('\t',second+1);
  if(first==std::string::npos||second==std::string::npos||third==std::string::npos)continue;
  size_t fourth=s.find('\t',third+1);
  Entry item{s.substr(0,first),clean_label(s.substr(first+1,second-first-1)),
    clean_label(s.substr(second+1,third-second-1)),s.substr(third+1,fourth==std::string::npos?std::string::npos:fourth-third-1),
    fourth==std::string::npos?"":s.substr(fourth+1)};
  if(!valid_slug(item.slug)||item.title.size()>120||item.category.size()>40)continue;
  if(item.url.rfind("https://motionbgs.com/dl/hd/",0)!=0)continue;
  if(item.preview.rfind("https://motionbgs.com/",0)!=0 && item.preview.rfind("https://",0)==0) item.preview="";
  next.push_back(std::move(item));
 }
 fclose(f);records_=std::move(next);
 preview_requested_.assign(records_.size(),false);
 installed_cache_.assign(records_.size(),false);
 for(int i=0;i<(int)records_.size();i++)
   installed_cache_[(size_t)i]=exists(wallpaper_path(i),1024)&&is_mp4(wallpaper_path(i));
 if(records_.empty()){report("Online catalog empty. Check MotionBGS availability.");return false;}
 report("MotionBGS catalog: "+std::to_string(records_.size())+" HD wallpapers");
 return true;
}
std::string Catalog::wallpaper_path(int i)const{
 if(i<0||i>=(int)records_.size())return {};
 return "/data/LiveShell/video/MotionBGS-"+records_[(size_t)i].slug+".mp4";
}
std::string Catalog::thumbnail_path(int i)const{
 if(i<0||i>=(int)records_.size())return {};
 return "/data/ThemeChanger/cache/MotionBGS-"+records_[(size_t)i].slug+".img";
}
bool Catalog::installed(int i)const{
 if(i<0||i>=(int)records_.size())return false;
 std::lock_guard<std::mutex> lock(mutex_);
 return installed_cache_[(size_t)i];
}
void Catalog::report(std::string message){std::lock_guard<std::mutex> g(mutex_);status_=std::move(message);}
std::string Catalog::status()const{std::lock_guard<std::mutex> g(mutex_);return status_;}
bool Catalog::start(int i,bool preview){
 if(i<0||i>=(int)records_.size())return false;
 auto& active=preview?preview_busy_:downloading_;
 if(active.exchange(true))return false;
 if(preview && records_[(size_t)i].preview.empty()){active=false;return false;}
 if(preview){
  std::lock_guard<std::mutex> g(mutex_);
  if(preview_requested_[(size_t)i]){active=false;return false;}
  preview_requested_[(size_t)i]=true;
 }else{percent_=0;report("Connecting to MotionBGS...");}
 auto* w=new Worker{this,i,preview};pthread_t thread{};
 if(pthread_create(&thread,nullptr,Catalog::run,w)!=0){
  delete w;active=false;report("Unable to start network thread");return false;
 }
 pthread_detach(thread);return true;
}
bool Catalog::download(int i){return start(i,false);}
bool Catalog::request_thumbnail(int i){return start(i,true);}
std::vector<int> Catalog::consume_ready(){
 std::lock_guard<std::mutex> g(mutex_);
 auto items=ready_;ready_.clear();return items;
}
void* Catalog::run(void* p){
 Worker job=*static_cast<Worker*>(p);delete static_cast<Worker*>(p);
 Catalog& self=*job.self;
 const auto& item=self.records_[(size_t)job.index];
 if(job.preview){
  make_directory("/data/ThemeChanger");make_directory("/data/ThemeChanger/cache");
  const std::string path=self.thumbnail_path(job.index);
  bool ok=is_image(path);
  if(!ok){
   std::atomic<int> prog{0};std::string err;
   ok=fetch_file(item.preview,path,prog,err);
   if(ok&&!is_image(path)){unlink(path.c_str());ok=false;}
  }
  if(ok){std::lock_guard<std::mutex> g(self.mutex_);self.ready_.push_back(job.index);}
  self.preview_busy_=false;return nullptr;
 }
 make_directory("/data/LiveShell");make_directory("/data/LiveShell/video");
 const std::string path=self.wallpaper_path(job.index);
 std::string err;bool ok=self.installed(job.index);
 if(!ok){
  self.report("Downloading "+item.title+" (1080p MP4)...");
  ok=fetch_file(item.url,path,self.percent_,err);
  if(ok&&!is_mp4(path)){unlink(path.c_str());ok=false;err="Server did not send a valid MP4";}
 }
 if(ok){
  {std::lock_guard<std::mutex> lock(self.mutex_);self.installed_cache_[(size_t)job.index]=true;}
  self.percent_=100;
  std::string applied;
  if(theme::activate_file("MotionBGS-"+item.slug+".mp4",applied))
   self.report(item.title+": downloaded, installed and applied");
  else self.report(item.title+": installed. "+applied);
 }else self.report("Download failed: "+err);
 self.downloading_=false;return nullptr;
}
}
