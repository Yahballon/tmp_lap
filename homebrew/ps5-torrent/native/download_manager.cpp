// SPDX-License-Identifier: GPL-3.0-or-later
#include "download_manager.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
namespace torrent {
namespace {
std::string trim(std::string s){
 size_t a=s.find_first_not_of(" \r\n\t");if(a==std::string::npos)return {};
 size_t b=s.find_last_not_of(" \r\n\t");return s.substr(a,b-a+1);
}
bool startsWith(const std::string& s,const std::string& p){return s.compare(0,p.size(),p)==0;}
bool safeField(const std::string& s){return !s.empty()&&s.size()<700&&s.find_first_of("|\r\n") ==std::string::npos;}
bool fileExists(const std::string& path){struct stat st{};return stat(path.c_str(),&st)==0&&S_ISREG(st.st_mode);}
bool ownedTorrentPath(const std::string& path){
 // Never delete bundled fixture metadata or arbitrary paths passed by users.
 return startsWith(path,storageRoot()+"/torrents/")||
        startsWith(path,storageRoot()+"/cache/")||
        startsWith(path,"/mnt/usb0/ps5-torrent/torrents/")||
        startsWith(path,"/mnt/usb0/ps5-torrent/cache/");
}
bool atomicWrite(const std::string& path,const std::string& body){
 std::string temp=path+".tmp";
 FILE* f=fopen(temp.c_str(),"wb");if(!f)return false;
 bool ok=fwrite(body.data(),1,body.size(),f)==body.size();
 if(fflush(f)!=0)ok=false;
 if(fclose(f)!=0)ok=false;
 if(!ok){remove(temp.c_str());return false;}
 if(rename(temp.c_str(),path.c_str())!=0){remove(temp.c_str());return false;}
 return true;
}
int clampInt(int n,int lo,int hi){return std::max(lo,std::min(n,hi));}
bool safeHash(const std::string& hash){
 if(hash.size()!=40)return false;
 for(char c:hash)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;
 return true;
}
bool copyMetadata(const std::string& source,const std::string& dest){
 FILE* f=fopen(source.c_str(),"rb");if(!f)return false;
 if(fseek(f,0,SEEK_END)!=0){fclose(f);return false;}
 long count=ftell(f);
 if(count<100||count>1024*1024){fclose(f);return false;}
 rewind(f);
 std::string bytes((size_t)count,'\0');
 const bool ok=fread(&bytes[0],1,bytes.size(),f)==bytes.size();
 fclose(f);
 return ok&&atomicWrite(dest,bytes);
}
}
std::string DownloadManager::torrentDirectory()const{return storageRoot()+"/torrents";}
std::string DownloadManager::iniPath()const{return storageRoot()+"/settings.ini";}
std::string DownloadManager::queuePath()const{return storageRoot()+"/cache/queue.db";}
std::string DownloadManager::sessionMarker()const{return storageRoot()+"/cache/session.active";}
std::string DownloadManager::recoveryMarker()const{return storageRoot()+"/cache/recovery.hold";}
std::string DownloadManager::metadataCachePath(const std::string& hash)const{
 if(!safeHash(hash))return {};
 return storageRoot()+"/cache/metainfo/"+hash+".torrent";
}
bool DownloadManager::resolveMetadata(Job& job,std::string& chosen){
 const std::string cache=metadataCachePath(job.hash);
 if(cache.empty())return false;
 auto matching=[&](const std::string& path)->bool{
  TorrentInfo info;std::string err;
  return fileExists(path)&&inspectTorrent(path,info,err)&&info.infoHash==job.hash;
 };
 // Prefer persistent metadata even when the app or USB source has disappeared.
 if(matching(cache)){chosen=cache;return true;}
 auto copyValid=[&](const std::string& path)->bool{
  if(!matching(path))return false;
  if(copyMetadata(path,cache)&&matching(cache)){chosen=cache;return true;}
  chosen=path;return true; // existing metadata still usable if cache is read-only
 };
 if(copyValid(job.path))return true;
 // Migrate v0.4.x jobs by matching the exact infohash, not merely the name.
 for(const auto& path:discoverTorrents()){
  if(!copyValid(path))continue;
  job.path=path;return true;
 }
 DIR* dir=opendir(torrentDirectory().c_str());
 if(dir){
  struct dirent* entry;
  while((entry=readdir(dir))!=nullptr){
   const std::string name=entry->d_name;
   if(name.size()<=8||name.size()>210||name.find("..")!=std::string::npos||
      name.compare(name.size()-8,8,".torrent")!=0)continue;
   const std::string path=torrentDirectory()+"/"+name;
   if(copyValid(path)){job.path=path;closedir(dir);return true;}
  }
  closedir(dir);
 }
 return false;
}
DownloadManager::DownloadManager(){
 createDirectories(storageRoot());
 mkdir(torrentDirectory().c_str(),0755);
 mkdir((storageRoot()+"/cache/metainfo").c_str(),0755);
 loadSettings();loadQueue();
 const std::string clean=storageRoot()+"/cache/last-exit.ok";
 // v0.4.1 crashed while automatically restarting persisted queue items.
 // Guard both unexpected crashes and the first migration of old queue data.
 recoveryMode_=fileExists(recoveryMarker())||
    (!jobs_.empty()&&(fileExists(sessionMarker())||!fileExists(clean)));
 remove(clean.c_str());
 FILE* marker=fopen(sessionMarker().c_str(),"wb");
 if(marker){fputs("Torrent Flow session active\n",marker);fclose(marker);}
 if(recoveryMode_){
  FILE* hold=fopen(recoveryMarker().c_str(),"wb");
  if(hold){fputs("Manual confirmation needed to resume queue\n",hold);fclose(hold);}
 }
 lastScan_=std::chrono::steady_clock::now()-std::chrono::seconds(settings_.scanSeconds+2);
}
DownloadManager::~DownloadManager(){stopAll();jobs_.clear();}
bool DownloadManager::resumeAfterRecovery(){
 if(!recoveryMode_)return false;
 recoveryMode_=false;
 remove(recoveryMarker().c_str());
 return true;
}
void DownloadManager::loadSettings(){
 FILE* f=fopen(iniPath().c_str(),"rb");
 if(!f){saveSettings();return;}
 char line[1024];
 while(fgets(line,sizeof(line),f)){
  std::string l=trim(line);
  if(l.empty()||l[0]=='#'||l[0]==';'||l[0]=='[')continue;
  size_t sep=l.find('=');
  if(sep==std::string::npos)continue;
  std::string key=trim(l.substr(0,sep)),value=trim(l.substr(sep+1));
  if(key=="auto_download")settings_.autoDownload=value=="1";
  else if(key=="delete_completed_torrents")settings_.deleteCompletedTorrents=value=="1";
  else if(key=="max_active_downloads")settings_.maxActive=clampInt(atoi(value.c_str()),1,9);
  else if(key=="scan_interval_seconds")settings_.scanSeconds=clampInt(atoi(value.c_str()),2,60);
  else if(key=="storage")settings_.usb=value=="usb0";
 }
 fclose(f);
}
void DownloadManager::saveSettings()const{
 std::string out="# Torrent Flow - PS5 settings\n[downloads]\n";
 out+="auto_download="+std::to_string(settings_.autoDownload?1:0)+"\n";
 out+="max_active_downloads="+std::to_string(settings_.maxActive)+"\n";
 out+="delete_completed_torrents="+std::to_string(settings_.deleteCompletedTorrents?1:0)+"\n";
 out+="scan_interval_seconds="+std::to_string(settings_.scanSeconds)+"\n";
 out+="storage="+std::string(settings_.usb?"usb0":"internal")+"\n";
 atomicWrite(iniPath(),out);
}
void DownloadManager::loadQueue(){
 FILE* f=fopen(queuePath().c_str(),"rb");if(!f)return;
 char line[2200];
 while(fgets(line,sizeof(line),f)&&jobs_.size()<96){
  std::string l=trim(line);
  if(startsWith(l,"#")||l.empty())continue;
  std::vector<std::string> fields;size_t first=0,last;
  while((last=l.find('|',first))!=std::string::npos){fields.push_back(l.substr(first,last-first));first=last+1;}
  fields.push_back(l.substr(first));
  if(fields.size()!=5)continue;
  if(!safeField(fields[0])||!safeField(fields[1])||!safeField(fields[2]))continue;
  if(fields[0].size()!=40)continue;
  auto job=std::make_unique<Job>();
  job->hash=fields[0];job->name=fields[1];job->path=fields[2];
  job->total=strtoull(fields[4].c_str(),nullptr,10);
  job->state=fields[3];
  if(job->state=="running"||job->state=="stopping")job->state="queued";
  if(job->state!="queued"&&job->state!="done"&&job->state!="failed"&&job->state!="cancelled")continue;
  // Missing sources can be recovered from verified persistent metadata.
  job->message=job->state=="done"?"Complete":job->state=="failed"?"Not running":job->state=="cancelled"?"Cancelled":"Queued";
  jobs_.push_back(std::move(job));
 }
 fclose(f);
 bool changed=false;
 for(auto& job:jobs_){
  if(job->state!="queued"&&job->state!="failed")continue;
  const std::string oldPath=job->path;
  std::string chosen;
  if(resolveMetadata(*job,chosen)){
   if(job->state=="failed"&&job->message=="Not running"){
    job->state="queued";job->message="Recovered metadata; queued";
    changed=true;
   }
   if(job->path!=oldPath)changed=true;
  }else if(job->state=="queued"){
   job->state="failed";job->message="Missing .torrent (source and cache)";
   changed=true;
  }
 }
 if(changed)saveQueue();
}
void DownloadManager::saveQueue()const{
 std::string out="# hash|name|torrent path|state|total bytes\n";
 for(const auto& job:jobs_){
  if(!safeField(job->hash)||!safeField(job->name)||!safeField(job->path))continue;
  out+=job->hash+"|"+job->name+"|"+job->path+"|"+job->state+"|"+std::to_string(job->total)+"\n";
 }
 atomicWrite(queuePath(),out);
}
bool DownloadManager::enqueue(const std::string& path,std::string* error,bool retryExisting){
 TorrentInfo info;std::string diagnostic;
 if(!inspectTorrent(path,info,diagnostic)){
  if(error)*error=diagnostic;return false;
 }
 // Do not let two different torrent hashes corrupt the same destination file.
 for(auto& existing:jobs_){
  if(existing->hash==info.infoHash){
   // A copied duplicate of an already completed torrent must not loop forever
   // in the watched inbox. Only remove metadata from our dedicated inbox.
   if(existing->state=="done"&&settings_.deleteCompletedTorrents&&
      startsWith(path,torrentDirectory()+"/"))safeDeleteSource(path);
   if(retryExisting&&(existing->state=="failed"||existing->state=="cancelled")){
    existing->path=path;existing->state="queued";existing->message="Queued";saveQueue();return true;
   }
   if(error)*error="Already in Downloads";
   return false;
  }
  if(existing->name==info.name){
   if(error)*error="Output filename conflicts with another torrent";
   return false;
  }
 }
 if(jobs_.size()>=96){if(error)*error="Queue is full (96 jobs)";return false;}
 if(!safeField(info.infoHash)||!safeField(info.name)||!safeField(path)){
  if(error)*error="Unsupported characters in torrent path or name";return false;
 }
 auto job=std::make_unique<Job>();
 job->name=info.name;job->hash=info.infoHash;job->path=path;job->total=info.bytes;
 // Never persist a queue job that only refers to a transient /app0 mount.
 const std::string cache=metadataCachePath(info.infoHash);
 if(cache.empty()||(!fileExists(cache)&&!copyMetadata(path,cache))){
  if(error)*error="Cannot preserve .torrent in /data/ps5-torrent/cache/metainfo";
  return false;
 }
 TorrentInfo cachedInfo;std::string verifyError;
 if(!inspectTorrent(cache,cachedInfo,verifyError)||cachedInfo.infoHash!=info.infoHash){
  if(!copyMetadata(path,cache)||!inspectTorrent(cache,cachedInfo,verifyError)||
     cachedInfo.infoHash!=info.infoHash){
   if(error)*error="Unable to verify preserved .torrent metadata";return false;
  }
 }
 job->message="Queued for download";
 jobs_.push_back(std::move(job));saveQueue();return true;
}
int DownloadManager::enqueueAll(const std::vector<std::string>& paths){
 int added=0;for(const auto& path:paths)if(enqueue(path))added++;
 return added;
}
void DownloadManager::cancel(size_t index){
 if(index>=jobs_.size())return;
 auto& job=*jobs_[index];
 if(job.state=="queued"){job.state="cancelled";job.message="Cancelled";saveQueue();}
 else if(job.state=="running"&&job.engine){job.state="stopping";job.message="Stopping...";job.engine->stop();saveQueue();}
}
void DownloadManager::retry(size_t index){
 if(index>=jobs_.size())return;
 Job& job=*jobs_[index];
 if(job.state!="failed"&&job.state!="cancelled")return;
 std::string chosen;
 if(!resolveMetadata(job,chosen)){
  job.message="Missing .torrent (source and cache)";return;
 }
 job.engine.reset();job.state="queued";job.message="Queued";saveQueue();
}
void DownloadManager::stopAll(){
 for(size_t i=0;i<jobs_.size();++i){
  auto& job=*jobs_[i];
  if((job.state=="running"||job.state=="stopping")&&job.engine)job.engine->stop();
 }
 // Engine destructors join their transfer threads. Preserve incomplete jobs for next launch.
 for(auto& job:jobs_){
  if(job->engine){job->engine.reset();if(job->state=="running"||job->state=="stopping"){job->state="queued";job->message="Queued after restart";}}
 }
 saveQueue();
 remove(sessionMarker().c_str());
 const std::string clean=storageRoot()+"/cache/last-exit.ok";
 FILE* f=fopen(clean.c_str(),"wb");
 if(f){fputs("Clean exit\n",f);fclose(f);}
 // Intentionally retain recovery.hold until the user explicitly resumes.
}
bool DownloadManager::safeDeleteSource(const std::string& path)const{
 if(!ownedTorrentPath(path))return false;
 if(path.find("..")!=std::string::npos||path.compare(path.size()-8,8,".torrent")!=0)return false;
 return remove(path.c_str())==0;
}
void DownloadManager::launchPending(){
 if(recoveryMode_)return;
 int active=0;
 for(auto& j:jobs_)if(j->state=="running"||j->state=="stopping")active++;
 for(auto& job:jobs_){
  if(active>=settings_.maxActive)break;
  if(job->state!="queued")continue;
  std::string metadata;
  if(!resolveMetadata(*job,metadata)){
   job->state="failed";job->message="Missing .torrent (source and cache)";
   saveQueue();continue;
  }
  job->engine=std::make_unique<Engine>();
  job->engine->setUsb(settings_.usb);
  // At nine transfers use three peers each instead of 72 open sessions.
  job->engine->setPeerWorkers(std::max(2,std::min(8,27/settings_.maxActive)));
  if(job->engine->start(metadata)){
   job->state="running";job->message="Starting...";active++;saveQueue();
  }else{
   job->state="failed";job->message="Could not start torrent";job->engine.reset();saveQueue();
  }
 }
}
void DownloadManager::scan(bool force){
 if(recoveryMode_)return;
 if(!settings_.autoDownload&&!force)return;
 const auto now=std::chrono::steady_clock::now();
 if(!force&&now-lastScan_<std::chrono::seconds(settings_.scanSeconds))return;
 lastScan_=now;
 DIR* dir=opendir(torrentDirectory().c_str());if(!dir)return;
 struct dirent* ent;
 bool dirty=false;
 while((ent=readdir(dir))!=nullptr){
  std::string name=ent->d_name;
  if(name.size()<9||name.size()>210||name.find("..")!=std::string::npos||
     name.compare(name.size()-8,8,".torrent")!=0)continue;
  std::string path=torrentDirectory()+"/"+name;
  struct stat st{};
  if(stat(path.c_str(),&st)!=0||!S_ISREG(st.st_mode)||st.st_size<100||st.st_size>1024*1024)continue;
  std::string fingerprint=std::to_string((long long)st.st_size)+"/"+std::to_string((long long)st.st_mtime);
  const bool stable=observed_[path]==fingerprint;
  observed_[path]=fingerprint;
  if(!stable)continue; // Skip files still being copied by FTP/USB.
  if(enqueue(path,nullptr,false))dirty=true;
 }
 closedir(dir);(void)dirty;
}
void DownloadManager::tick(){
 // Single-threaded scheduler: call from the PS5 frame loop, never background thread.
 if(inTick_)return;inTick_=true;
 bool changed=false;
 for(auto& job:jobs_){
  if(!job->engine)continue;
  const Snapshot snap=job->engine->snapshot();
  job->message=snap.status;
  if(!snap.running&&(job->state=="running"||job->state=="stopping")){
   const bool wasStopping=job->state=="stopping";
   job->state=snap.done?"done":wasStopping?"cancelled":"failed";
   job->message=snap.status;
   job->engine.reset();
   if(snap.done&&settings_.deleteCompletedTorrents){
    safeDeleteSource(job->path);
    const std::string cache=metadataCachePath(job->hash);
    if(!cache.empty())remove(cache.c_str());
   }
   changed=true;
  }
 }
 if(changed)saveQueue();
 scan(false);
 launchPending();
 inTick_=false;
}
std::vector<JobInfo> DownloadManager::jobs()const{
 std::vector<JobInfo> out;
 for(const auto& job:jobs_){
  JobInfo item;
  item.name=job->name;item.hash=job->hash;item.source=job->path;
  item.phase=job->state;item.status=job->message;item.total=job->total;
  item.queued=job->state=="queued";item.running=job->state=="running"||job->state=="stopping";
  item.completedJob=job->state=="done";item.failed=job->state=="failed";item.cancelled=job->state=="cancelled";
  if(job->engine){
   auto snap=job->engine->snapshot();
   item.total=snap.totalBytes?snap.totalBytes:item.total;
   item.completed=snap.completedBytes;item.speed=snap.downBps;
   item.peers=snap.peerCount;item.activePeers=snap.activePeers;item.status=snap.status;
  }else if(item.completedJob)item.completed=item.total;
  out.push_back(std::move(item));
 }
 return out;
}
void DownloadManager::setAutoDownload(bool v){settings_.autoDownload=v;saveSettings();lastScan_=std::chrono::steady_clock::now()-std::chrono::seconds(settings_.scanSeconds+2);}
void DownloadManager::setMaxActive(int n){settings_.maxActive=clampInt(n,1,9);saveSettings();}
void DownloadManager::setDeleteCompleted(bool v){settings_.deleteCompletedTorrents=v;saveSettings();}
bool DownloadManager::setUsbStorage(bool enabled){
 for(auto& j:jobs_)if(j->state=="running"||j->state=="stopping")return false;
 if(enabled){struct stat st{};if(stat("/mnt/usb0",&st)!=0||!S_ISDIR(st.st_mode))return false;}
 settings_.usb=enabled;saveSettings();return true;
}
void DownloadManager::setScanSeconds(int seconds){settings_.scanSeconds=clampInt(seconds,2,60);saveSettings();}
}
