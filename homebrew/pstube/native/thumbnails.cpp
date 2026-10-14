// SPDX-License-Identifier: GPL-3.0-or-later
// Asynchronous native thumbnail cache. Video ID is validated; URL is not user-controlled.
#include "pstube.hpp"
#include "transfer.hpp"
#include <atomic>
#include <memory>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
namespace pstube {
namespace {
std::atomic<bool> working{false};
struct Request {std::vector<std::string> ids;};
void* worker(void* data){
 std::unique_ptr<Request> req(static_cast<Request*>(data));
 ::mkdir("/data/pstube",0700);
 ::mkdir("/data/pstube/thumbs",0700);
 for(const std::string& id:req->ids){
  if(!valid_id(id))continue;
  std::string target="/data/pstube/thumbs/"+id+".jpg";
  struct stat st{};
  if(::stat(target.c_str(),&st)==0&&st.st_size>1024)continue;
  std::atomic<int> progress{0};std::string error;
  fetch_mp4("https://i.ytimg.com/vi/"+id+"/mqdefault.jpg",target,progress,error);
 }
 working.store(false);return nullptr;
}
}
void queue_thumbnails(const std::vector<Video>& videos){
 if(videos.empty()||working.exchange(true))return;
 auto* req=new Request;
 for(const auto& v:videos){if(req->ids.size()>=18)break;if(valid_id(v.id))req->ids.push_back(v.id);}
 pthread_t t{};pthread_attr_t a;pthread_attr_init(&a);pthread_attr_setstacksize(&a,1024*1024);
 int rc=pthread_create(&t,&a,worker,req);
 pthread_attr_destroy(&a);
 if(rc){delete req;working.store(false);}
 else pthread_detach(t);
}
}
