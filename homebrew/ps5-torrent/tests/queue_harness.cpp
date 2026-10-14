// Real C++ queue lifecycle integration harness.
#include "download_manager.hpp"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
int main(int argc,char** argv){
 if(argc<2)return 2;
 const std::string mode=argv[1];
 if(mode=="--multi"&&argc==4){
  torrent::DownloadManager m;
  m.setAutoDownload(false);m.setMaxActive(2);
  std::string error;
  if(!m.enqueue(argv[2],&error)){std::cerr<<"enqueue first: "<<error<<'\n';return 3;}
  if(!m.enqueue(argv[3],&error)){std::cerr<<"enqueue second: "<<error<<'\n';return 4;}
  bool simultaneously=false;
  for(int i=0;i<300;i++){
   m.tick();auto jobs=m.jobs();int active=0,done=0;
   for(auto& job:jobs){active+=(int)job.running;done+=(int)job.completedJob;
    if(job.failed){std::cerr<<"FAILED "<<job.name<<": "<<job.status<<'\n';return 5;}}
   if(active>=2)simultaneously=true;
   if(done>=2){std::cout<<"MULTI DONE active_together="<<simultaneously<<'\n';return simultaneously?0:6;}
   std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  std::cerr<<"Queue jobs did not finish\n";return 7;
 }
 if(mode=="--auto"&&argc==2){
  torrent::DownloadManager m;
  m.setAutoDownload(true);m.setMaxActive(2);m.setDeleteCompleted(true);m.setScanSeconds(2);
  int discovered=0;
  for(int i=0;i<350;i++){
   m.tick();
   auto jobs=m.jobs();
   if(!jobs.empty())discovered=1;
   int done=0;
   for(auto& job:jobs){done+=(int)job.completedJob;
    if(job.failed){std::cerr<<"AUTO FAILED "<<job.status<<'\n';return 8;}}
   if(done>=1){std::cout<<"AUTO DONE detected="<<discovered<<" jobs="<<jobs.size()<<'\n';return 0;}
   std::this_thread::sleep_for(std::chrono::milliseconds(70));
  }
  std::cerr<<"Auto download scan did not complete\n";return 9;
 }
 if(mode=="--history"&&argc==2){
  torrent::DownloadManager m;
  auto jobs=m.jobs();
  int complete=0;
  for(auto& j:jobs)complete+=(int)j.completedJob;
  std::cout<<"PERSISTED jobs="<<jobs.size()<<" complete="<<complete<<'\n';
  return jobs.size()>=1&&complete>=1?0:10;
 }
 return 11;
}
