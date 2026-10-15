// SPDX-License-Identifier: GPL-3.0-or-later
// Bounded concurrent thumbnail cache with real filesystem and network diagnostics.
#include "pstube.hpp"
#include "transfer.hpp"
#include <algorithm>
#include <atomic>
#include <deque>
#include <mutex>
#include <pthread.h>
#include <set>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cerrno>
#include <cstring>
namespace pstube {
namespace {
std::mutex lock;
std::deque<std::string> pending;
std::set<std::string> scheduled;
int active=0;
int successful=0;
int failed=0;
std::string last_error;
std::string cache_dir;
bool writable(const char* root){
 if(mkdir(root,0755)!=0&&errno!=EEXIST)return false;
 const std::string probe=std::string(root)+"/.writecheck";
 FILE* f=fopen(probe.c_str(),"wb");
 if(!f)return false;
 const bool ok=fputc('X',f)!=EOF&&fclose(f)==0;
 unlink(probe.c_str());
 return ok;
}
const std::string& directory(){
 static const std::string path=[]{
  mkdir("/data/pstube",0755);
  if(writable("/data/pstube/thumbs"))return std::string("/data/pstube/thumbs");
  mkdir("/download0/pstube",0755);
  if(writable("/download0/pstube/thumbs"))return std::string("/download0/pstube/thumbs");
  return std::string();
 }();
 return path;
}
void* worker(void*){
 for(;;){
  std::string id;
  {
   std::lock_guard<std::mutex> guard(lock);
   if(pending.empty()){--active;return nullptr;}
   id=pending.front();pending.pop_front();
  }
  const std::string dest=directory()+"/"+id+".jpg";
  struct stat st{};
  bool ok=false;std::string error;
  if(directory().empty())error="Neither /data/pstube/thumbs nor /download0/pstube/thumbs writable";
  else if(stat(dest.c_str(),&st)==0&&st.st_size>1024)ok=true;
  else{
   std::atomic<int> progress{0};
   // Exact thumbnail video ID, never derived from an arbitrary URL.
   ok=fetch_mp4("https://i.ytimg.com/vi/"+id+"/mqdefault.jpg",dest,progress,error);
   // Reject HTTP success pages masquerading as .jpg.
   if(ok){
    FILE* file=fopen(dest.c_str(),"rb");
    unsigned char magic[3]{};
    bool jpeg=file&&fread(magic,1,3,file)==3&&
      magic[0]==0xff&&magic[1]==0xd8&&magic[2]==0xff;
    if(file)fclose(file);
    if(!jpeg){unlink(dest.c_str());ok=false;error="Thumbnail is not a JPEG";}
   }
  }
  {
   std::lock_guard<std::mutex> guard(lock);
   if(ok)++successful;
   else{++failed;last_error=id+": "+(error.empty()?"Unknown thumbnail error":error);}
   scheduled.erase(id);
  }
 }
}
}
std::string thumbnail_file(const std::string& videoId){
 return valid_id(videoId)&&!directory().empty()?directory()+"/"+videoId+".jpg":std::string();
}
ThumbnailHealth thumbnail_health(){
 ThumbnailHealth h;
 h.directory=directory();
 std::lock_guard<std::mutex> guard(lock);
 h.successful=successful;h.failed=failed;
 h.pending=(int)pending.size()+active;
 h.lastError=last_error;
 return h;
}
void queue_thumbnails(const std::vector<Video>& videos){
 int start=0;
 {
  std::lock_guard<std::mutex> guard(lock);
  for(const auto& video:videos){
   if(!valid_id(video.id)||scheduled.count(video.id))continue;
   const std::string path=thumbnail_file(video.id);
   struct stat st{};
   if(!path.empty()&&stat(path.c_str(),&st)==0&&st.st_size>1024)continue;
   if(pending.size()>=80)break;
   pending.push_back(video.id);scheduled.insert(video.id);
  }
  if(!pending.empty()){start=std::min(3-active,(int)pending.size());active+=start;}
 }
 for(int i=0;i<start;++i){
  pthread_t t{};pthread_attr_t a;
  pthread_attr_init(&a);pthread_attr_setstacksize(&a,1024*1024);
  const int rc=pthread_create(&t,&a,worker,nullptr);pthread_attr_destroy(&a);
  if(rc){std::lock_guard<std::mutex> guard(lock);--active;last_error="pthread_create failed";}
  else pthread_detach(t);
 }
}
} // namespace pstube
