// Real C++ queue lifecycle integration harness.
#include "download_manager.hpp"
#include <chrono>
#include <cstdlib>
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
 if(mode=="--nine"&&argc==11){
  torrent::DownloadManager m;
  m.setAutoDownload(false);m.setMaxActive(9);
  if(m.settings().maxActive!=9)return 20;
  for(int j=2;j<11;j++){
   std::string error;
   if(!m.enqueue(argv[j],&error)){std::cerr<<"NINE enqueue "<<j<<" "<<error<<'\n';return 21;}
  }
  m.tick();
  int concurrently=0;
  for(const auto& job:m.jobs())concurrently+=(int)job.running;
  if(concurrently!=9){std::cerr<<"Only "<<concurrently<<" of 9 launched\n";return 22;}
  for(int i=0;i<500;i++){
   m.tick();int done=0;
   for(const auto& job:m.jobs()){
    done+=(int)job.completedJob;
    if(job.failed){std::cerr<<"NINE failed "<<job.name<<" "<<job.status<<'\n';return 23;}
   }
   if(done==9){std::cout<<"NINE DONE active_at_once="<<concurrently<<"\n";return 0;}
   std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  return 24;
 }
 if(mode=="--metadata-seed"&&argc==3){
  torrent::DownloadManager m;
  std::string error;
  if(!m.enqueue(argv[2],&error)){std::cerr<<"metadata seed: "<<error<<'\n';return 25;}
  // Clean exit stores the queue without starting network transfers.
  return 0;
 }
 if(mode=="--metadata-recover"&&argc==2){
  torrent::DownloadManager m;
  auto jobs=m.jobs();
  if(jobs.size()!=1||!jobs[0].queued||m.isRecoveryMode()){
   std::cerr<<"Metadata missing after original source removal, status="<<(jobs.empty()?"no jobs":jobs[0].status)<<'\n';return 26;
  }
  m.tick();
  for(int i=0;i<300;i++){
   m.tick();jobs=m.jobs();
   if(jobs[0].failed){std::cerr<<"Recovered job failed: "<<jobs[0].status<<'\n';return 27;}
   if(jobs[0].completedJob){std::cout<<"METADATA CACHE restored and downloaded\n";return 0;}
   std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  return 28;
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
 if(mode=="--cancel-auto"&&argc==2){
  torrent::DownloadManager m;
  m.setAutoDownload(true);m.setMaxActive(1);m.setScanSeconds(2);
  bool requested=false,observedCancelled=false;
  auto start=std::chrono::steady_clock::now();
  while(std::chrono::steady_clock::now()-start<std::chrono::seconds(8)){
   m.tick();
   auto jobs=m.jobs();
   if(jobs.size()==1&&jobs[0].running&&!requested){
    m.cancel(0);requested=true;
   }
   if(requested&&jobs.size()==1&&jobs[0].cancelled)observedCancelled=true;
   if(observedCancelled&&jobs.size()==1&&jobs[0].running){
    std::cerr<<"Cancelled auto job was restarted by folder watcher\n";return 12;
   }
   std::this_thread::sleep_for(std::chrono::milliseconds(65));
  }
  auto jobs=m.jobs();
  std::cout<<"AUTO CANCEL requested="<<requested<<" persisted_cancelled="
           <<observedCancelled<<" phase="<<(jobs.empty()?"none":jobs[0].phase)<<'\n';
  return requested&&observedCancelled&&jobs.size()==1&&jobs[0].cancelled?0:13;
 }
 if(mode=="--crash-create"&&argc==3){
  torrent::DownloadManager m;
  std::string error;
  if(!m.enqueue(argv[2],&error)){std::cerr<<"Crash fixture enqueue failed "<<error<<'\n';return 14;}
  // Simulates a kernel trap: destructors never run, session.active remains.
  std::_Exit(0);
 }
 if(mode=="--check-recovery"&&argc==2){
  torrent::DownloadManager m;
  auto jobs=m.jobs();
  if(!m.isRecoveryMode()||jobs.empty()||!jobs[0].queued){
   std::cerr<<"Missing crash recovery mode\n";return 15;
  }
  for(int i=0;i<5;i++)m.tick();
  jobs=m.jobs();
  if(!jobs[0].queued||jobs[0].running){
   std::cerr<<"Recovery incorrectly launched persisted job\n";return 16;
  }
  std::cout<<"RECOVERY queue paused\n";
  return 0; // normal clean shutdown must NOT erase recovery.hold
 }
 if(mode=="--resume-recovery"&&argc==2){
  torrent::DownloadManager m;
  if(!m.isRecoveryMode()||!m.resumeAfterRecovery()||m.isRecoveryMode())return 17;
  std::cout<<"RECOVERY manually unlocked\n";
  return 0;
 }
 if(mode=="--check-clean"&&argc==2){
  torrent::DownloadManager m;
  if(m.isRecoveryMode())return 18;
  std::cout<<"CLEAN queue restored without crash mode\n";
  return 0;
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
