// SPDX-License-Identifier: GPL-3.0-or-later
// Persistent thumbnail queue: multiple concurrent PS5 HTTPS workers.
// Only genuine video IDs from the API are accepted. No fabricated images.
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
namespace pstube {
namespace {
std::mutex lock;
std::deque<std::string> pending;
std::set<std::string> scheduled;
int active=0;
void* worker(void*){
 ::mkdir("/data/pstube",0700);
 ::mkdir("/data/pstube/thumbs",0700);
 for(;;){
  std::string id;
  {
   std::lock_guard<std::mutex> guard(lock);
   if(pending.empty()){--active;return nullptr;}
   id=pending.front();pending.pop_front();
  }
  const std::string target="/data/pstube/thumbs/"+id+".jpg";
  struct stat st{};
  if(::stat(target.c_str(),&st)!=0||st.st_size<1024){
   std::atomic<int> progress{0};
   std::string error;
   // Public video-thumbnail CDN, never an arbitrary user-supplied URL.
   fetch_mp4("https://i.ytimg.com/vi/"+id+"/mqdefault.jpg",target,progress,error);
  }
  std::lock_guard<std::mutex> guard(lock);
  scheduled.erase(id);
 }
}
}
void queue_thumbnails(const std::vector<Video>& videos){
 int start=0;
 {
  std::lock_guard<std::mutex> guard(lock);
  for(const auto& video:videos){
   if(!valid_id(video.id)||scheduled.count(video.id))continue;
   struct stat st{};
   const std::string path="/data/pstube/thumbs/"+video.id+".jpg";
   if(::stat(path.c_str(),&st)==0&&st.st_size>1024)continue;
   if(pending.size()>=80)break;
   pending.push_back(video.id);scheduled.insert(video.id);
  }
  if(!pending.empty()){
   start=std::min(3-active,(int)pending.size());
   active+=start;
  }
 }
 for(int i=0;i<start;++i){
  pthread_t t{};pthread_attr_t a;
  pthread_attr_init(&a);pthread_attr_setstacksize(&a,1024*1024);
  int rc=pthread_create(&t,&a,worker,nullptr);
  pthread_attr_destroy(&a);
  if(rc){std::lock_guard<std::mutex> guard(lock);--active;}
  else pthread_detach(t);
 }
}
} // namespace pstube
