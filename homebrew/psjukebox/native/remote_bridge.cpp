// SPDX-License-Identifier: GPL-3.0-or-later
// Dedicated PSJukebox read-only bridge. ELF stays outside app sandbox;
// app receives limited catalog entries/files on localhost, and caches
// playable music into its writable storage.
#include "remote_bridge.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <map>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>
namespace jbx {
namespace {
#ifndef JUKEBOX_BRIDGE_PORT
#define JUKEBOX_BRIDGE_PORT 9188
#endif
#ifndef JUKEBOX_DATA_ROOT
#define JUKEBOX_DATA_ROOT "/data/PSJukebox"
#endif
constexpr int kPort=JUKEBOX_BRIDGE_PORT;
constexpr uint64_t kMaxAudio=8ULL*1024*1024*1024,kMaxImage=12ULL*1024*1024;
std::string lower(std::string s){
 for(auto& c:s)c=(char)std::tolower((unsigned char)c);
 return s;
}
bool name_ok(const std::string& name){
 if(name.empty()||name.size()>180||name[0]=='.')return false;
 for(unsigned char c:name)if(c<32||c==127||c=='/'||c=='\\')return false;
 return true;
}
bool relative_ok(const std::string& s){
 if(s.empty()||s.size()>510||s[0]=='/'||s.back()=='/')return false;
 size_t from=0;
 while(from<s.size()){
  auto next=s.find('/',from),end=next==std::string::npos?s.size():next;
  if(!name_ok(s.substr(from,end-from)))return false;
  if(next==std::string::npos)break;
  from=next+1;
 }
 return true;
}
std::vector<std::string> segments(const std::string& s){
 std::vector<std::string> out;size_t from=0;
 while(from<s.size()){
  const auto next=s.find('/',from);
  out.push_back(s.substr(from,next==std::string::npos?next:next-from));
  if(next==std::string::npos)break;
  from=next+1;
 }
 return out;
}
bool remote_parts(const std::string& path,unsigned& root,std::string& rel){
 if(path.rfind("jbox://",0)!=0)return false;
 auto slash=path.find('/',7);
 if(slash==std::string::npos||slash==7||slash>10)return false;
 root=0;
 for(size_t i=7;i<slash;i++){
  if(path[i]<'0'||path[i]>'9')return false;
  root=root*10+(unsigned)(path[i]-'0');
 }
 if(root>=32)return false;
 rel=path.substr(slash+1);
 return relative_ok(rel);
}
std::string virtual_path(unsigned root,const std::string& relative){
 return "jbox://"+std::to_string(root)+"/"+relative;
}
int connect_to(int port){
 int fd=socket(AF_INET,SOCK_STREAM,0);if(fd<0)return -1;
 timeval tv{10,0};
 setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
 setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv));
 sockaddr_in dst{};dst.sin_family=AF_INET;dst.sin_port=htons((uint16_t)port);
 dst.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
 if(connect(fd,(sockaddr*)&dst,sizeof(dst))!=0){close(fd);return -1;}
 return fd;
}
bool send_all(int fd,const char* data,size_t n){
 for(size_t off=0;off<n;){ssize_t w=send(fd,data+off,n-off,0);if(w<=0)return false;off+=(size_t)w;}
 return true;
}
bool line(int fd,std::string& result,size_t max=600){
 result.clear();char ch=0;
 while(result.size()<max){
  if(recv(fd,&ch,1,0)!=1)return false;
  if(ch=='\n')return true;
  if(ch=='\r'||ch==0)return false;
  result.push_back(ch);
 }
 return false;
}
bool launch_helper(std::string& error){
 FILE* f=fopen("/app0/jbxscan.elf","rb");
 if(!f)f=fopen("/data/homebrew/PPSA99947/jbxscan.elf","rb");
 if(!f){error="Packaged USB helper jbxscan.elf missing";return false;}
 int fd=connect_to(9021);
 if(fd<0){fclose(f);error="USB helper requires running elfldr 127.0.0.1:9021";return false;}
 char buf[16384];bool ok=true;
 for(;;){
  size_t got=fread(buf,1,sizeof(buf),f);
  if(!got){if(ferror(f))ok=false;break;}
  if(!send_all(fd,buf,got)){ok=false;break;}
 }
 fclose(f);
 if(ok)shutdown(fd,SHUT_WR);
 // elfldr may keep connection open while the ELF is running. Close here;
 // polling the independent loopback service is authoritative.
 close(fd);
 if(!ok){error="ELF transfer to elfldr failed";return false;}
 for(int i=0;i<50;i++){
  int probe=connect_to(kPort);
  if(probe>=0){close(probe);return true;}
  usleep(100000);
 }
 error="ELF started, but jukebox helper port 9188 unavailable";
 return false;
}
bool read_catalog(std::string& catalog,std::string& error){
 int fd=connect_to(kPort);
 if(fd<0){error="Helper service not reachable";return false;}
 bool ok=send_all(fd,"LIST\n",5);
 if(ok){
  std::string row;
  if(!line(fd,row)||row!="JBOX1")ok=false;
  else{
   catalog.clear();catalog+="JBOX1\n";
   for(unsigned i=0;i<16000;i++){
    if(!line(fd,row)){ok=false;break;}
    if(catalog.size()+row.size()+1>1600000){ok=false;break;}
    catalog+=row+"\n";
    if(row=="DONE")break;
    if(i==15999)ok=false;
   }
   if(catalog.size()<5||catalog.compare(catalog.size()-5,5,"DONE\n"))ok=false;
  }
 }
 close(fd);
 if(!ok)error="Invalid or truncated helper catalog";
 return ok;
}
bool read_header(int fd,uint64_t& bytes){
 std::string head;if(!line(fd,head,90))return false;
 if(head.rfind("DATA ",0)!=0)return false;
 std::string num=head.substr(5);
 if(num.empty()||num.size()>20)return false;
 bytes=0;
 for(char ch:num){
  if(ch<'0'||ch>'9'||bytes>kMaxAudio/10)return false;
  bytes=bytes*10+(unsigned)(ch-'0');
 }
 return bytes<=kMaxAudio;
}
bool request(unsigned root,const std::string& relative,uint64_t offset,
             int& fd,uint64_t& size,std::string& error){
 if(!relative_ok(relative)){error="Invalid relative music path";return false;}
 fd=connect_to(kPort);
 if(fd<0){error="USB helper disconnected; rescan to restart it";return false;}
 const std::string cmd="FETCH "+std::to_string(root)+" "+std::to_string(offset)+" "+relative+"\n";
 if(!send_all(fd,cmd.data(),cmd.size())||!read_header(fd,size)){
  close(fd);fd=-1;error="Helper could not open music/image; file moved or blocked";
  return false;
 }
 return true;
}
bool read_small(const std::string& path,size_t limit,std::string& data,std::string& error){
 unsigned root=0;std::string rel;
 if(!remote_parts(path,root,rel)){error="Invalid helper file reference";return false;}
 int fd=-1;uint64_t n=0;
 if(!request(root,rel,0,fd,n,error))return false;
 if(n>limit||n==0){close(fd);error="Image/metadata file exceeds size limit";return false;}
 data.resize((size_t)n);
 size_t offset=0;
 while(offset<data.size()){
  ssize_t got=recv(fd,&data[offset],data.size()-offset,0);
  if(got<=0)break;offset+=(size_t)got;
 }
 close(fd);
 if(offset!=data.size()){data.clear();error="Incomplete image/metadata transfer";return false;}
 return true;
}
struct Entry{unsigned root;uint64_t size;std::string relative;};
int picture_rank(const std::string& p,const std::string& base){
 const std::string name=lower(p);
 return name==base+".png"?0:name==base+".jpg"?1:name==base+".jpeg"?2:99;
}
void update_picture(std::string& found,int& rank,const Entry& entry,const std::string& name,
                    const std::string& base){
 int candidate=picture_rank(name,base);
 if(candidate>=rank||entry.size<8||entry.size>kMaxImage)return;
 rank=candidate;found=virtual_path(entry.root,entry.relative);
}
uint64_t hash_path(const std::string& path){
 uint64_t h=14695981039346656037ULL;
 for(unsigned char c:path){h^=c;h*=1099511628211ULL;}
 return h;
}
}
bool is_remote(const std::string& path){unsigned r;std::string p;return remote_parts(path,r,p);}
bool image_bytes(const std::string& path,std::string& data,std::string& error){
 return read_small(path,(size_t)kMaxImage,data,error);
}
CatalogResult scan(){
 CatalogResult result;std::string catalog,error;
 if(!read_catalog(catalog,error)){
  if(!launch_helper(error)||!read_catalog(catalog,error)){
   result.status=error;return result;
  }
 }
 result.helperRunning=true;
 std::vector<Entry> files;
 std::istringstream in(catalog);
 std::string row;std::getline(in,row);
 while(std::getline(in,row)){
  if(row.rfind("R ",0)==0||row.rfind("ERROR ",0)==0){
   result.status+=row+"; ";continue;
  }
  if(row.rfind("F ",0)!=0)continue;
  size_t start=row.find(' ',2);
  if(start==std::string::npos)continue;
  size_t startPath=row.find(' ',start+1);
  if(startPath==std::string::npos)continue;
  const auto id=row.substr(2,start-2),sz=row.substr(start+1,startPath-start-1),rel=row.substr(startPath+1);
  if(id.empty()||id.size()>2||sz.empty()||sz.size()>20||!relative_ok(rel))continue;
  unsigned rid=0;uint64_t n=0;bool valid=true;
  for(char ch:id){if(ch<'0'||ch>'9'){valid=false;break;}rid=rid*10+(ch-'0');}
  for(char ch:sz){if(ch<'0'||ch>'9'||n>kMaxAudio/10){valid=false;break;}n=n*10+(ch-'0');}
  if(!valid||rid>=32||n==0||n>kMaxAudio)continue;
  files.push_back({rid,n,rel});
 }
 std::map<std::string,size_t> artistIndices;
 std::vector<std::string> themeFiles;
 std::vector<int> logoRank,bgRank;
 std::vector<std::vector<int>> coverRanks,albumBgRanks;
 for(const auto& entry:files){
  const auto p=segments(entry.relative);
  if(p.size()!=2&&p.size()!=3)continue;
  const std::string key=std::to_string(entry.root)+":"+lower(p[0]);
  size_t a=0;auto it=artistIndices.find(key);
  if(it==artistIndices.end()){
   a=result.library.artists.size();
   if(a>=256)continue;
   artistIndices.emplace(key,a);
   jukebox::Artist artist;artist.name=p[0];artist.path=virtual_path(entry.root,p[0]);
   result.library.artists.push_back(std::move(artist));
   logoRank.push_back(99);bgRank.push_back(99);themeFiles.emplace_back();
   coverRanks.emplace_back();albumBgRanks.emplace_back();
  }else a=it->second;
  auto& artist=result.library.artists[a];
  if(p.size()==2){
   update_picture(artist.logo,logoRank[a],entry,p[1],"logo");
   update_picture(artist.background,bgRank[a],entry,p[1],"background");
   if(lower(p[1])=="theme.ini"&&entry.size<=4096)
    themeFiles[a]=virtual_path(entry.root,entry.relative);
  }else{
   const std::string extension=lower(p[2]);
   const bool musical=jukebox::is_music(extension);
   const bool picture=picture_rank(extension,"cover")<99||picture_rank(extension,"background")<99;
   if(!musical&&!picture)continue;
   size_t albumIndex=artist.albums.size();
   for(size_t i=0;i<artist.albums.size();i++)if(lower(artist.albums[i].name)==lower(p[1])){
    albumIndex=i;break;
   }
   if(albumIndex==artist.albums.size()){
    if(albumIndex>=128)continue;
    jukebox::Album album;album.name=p[1];album.path=virtual_path(entry.root,p[0]+"/"+p[1]);
    artist.albums.push_back(std::move(album));
    coverRanks[a].push_back(99);albumBgRanks[a].push_back(99);
   }
   auto& album=artist.albums[albumIndex];
   if(musical){
    if(album.tracks.size()>=400)continue;
    auto title=p[2];auto dot=title.find_last_of('.');if(dot!=std::string::npos)title.resize(dot);
    for(char& ch:title)if(ch=='_')ch=' ';
    album.tracks.push_back({title,virtual_path(entry.root,entry.relative),entry.size});
    result.library.trackCount++;
   }else{
    update_picture(album.cover,coverRanks[a][albumIndex],entry,p[2],"cover");
    update_picture(album.background,albumBgRanks[a][albumIndex],entry,p[2],"background");
   }
  }
 }
 for(size_t i=0;i<result.library.artists.size();i++){
  auto& artist=result.library.artists[i];
  if(!themeFiles[i].empty()){
   std::string bytes,err;
   if(read_small(themeFiles[i],4096,bytes,err))
     artist.theme=jukebox::parse_theme_text(bytes);
  }
  for(auto& album:artist.albums){
   std::sort(album.tracks.begin(),album.tracks.end(),[](const auto& a,const auto& b){return lower(a.name)<lower(b.name);});
  }
  artist.albums.erase(std::remove_if(artist.albums.begin(),artist.albums.end(),
    [](const auto& al){return al.tracks.empty();}),artist.albums.end());
  std::sort(artist.albums.begin(),artist.albums.end(),[](const auto& a,const auto& b){
   return lower(a.name)<lower(b.name);
  });
 }
 result.library.artists.erase(std::remove_if(result.library.artists.begin(),result.library.artists.end(),
    [](const auto& artist){return artist.albums.empty();}),result.library.artists.end());
 std::sort(result.library.artists.begin(),result.library.artists.end(),[](const auto& a,const auto& b){
  return lower(a.name)<lower(b.name);
 });
 result.status+="HELPER artists="+std::to_string(result.library.artists.size())+
  " tracks="+std::to_string(result.library.trackCount);
 result.library.report=result.status;
 return result;
}
bool stage_track(const std::string& path,uint64_t expected,std::atomic<bool>& cancelled,
                 std::string& local,std::string& error,std::atomic<uint64_t>* copied){
 local.clear();if(copied)copied->store(0);unsigned root;std::string rel;
 if(!remote_parts(path,root,rel)||expected<1024||expected>kMaxAudio||
    !jukebox::is_music(rel)){error="Invalid remote audio reference";return false;}
 const std::string base=JUKEBOX_DATA_ROOT,cache=base+"/Cache";
 mkdir(base.c_str(),0755);mkdir(cache.c_str(),0755);
 char hash[32];std::snprintf(hash,sizeof(hash),"%016llx",(unsigned long long)hash_path(path));
 const std::string extension=rel.substr(rel.find_last_of('.'));
 local=cache+"/"+hash+extension;
 struct stat st{};
 if(stat(local.c_str(),&st)==0&&S_ISREG(st.st_mode)&&(uint64_t)st.st_size==expected)return true;
 const std::string part=local+".part";
 FILE* f=fopen(part.c_str(),"wb");
 if(!f){error="Cannot write /data/PSJukebox/Cache";return false;}
 int fd=-1;uint64_t received=0;
 bool ok=request(root,rel,0,fd,received,error);
 if(ok&&received!=expected){error="Remote music size changed; rescan library";ok=false;}
 uint64_t left=received;
 char buffer[32768];
 while(ok&&left&&!cancelled.load()){
  size_t ask=left<sizeof(buffer)?(size_t)left:sizeof(buffer);
  ssize_t n=recv(fd,buffer,ask,0);
  if(n<=0||fwrite(buffer,1,(size_t)n,f)!=(size_t)n){error="Audio transfer interrupted";ok=false;break;}
  left-=(size_t)n;
  if(copied)copied->store(expected-left);
 }
 if(cancelled.load()){error="Music transfer cancelled";ok=false;}
 if(fd>=0)close(fd);
 if(fflush(f)!=0)ok=false;
 if(fclose(f)!=0)ok=false;
 if(!ok||left){remove(part.c_str());return false;}
 if(rename(part.c_str(),local.c_str())!=0){remove(part.c_str());error="Could not finalize audio cache";return false;}
 return true;
}
} // namespace jbx
