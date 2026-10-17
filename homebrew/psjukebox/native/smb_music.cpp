// SPDX-License-Identifier: GPL-3.0-or-later
// Native SMB2/SMB3 read-only client. Configure /data/PSJukebox/smb.ini.
// Listing is small/metadata-only; music and .album files are downloaded only
// when the song is played or the packed album is imported.
#include "smb_music.hpp"
#include "album_archive.hpp"
#include <smb2/libsmb2.h>
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <map>
#include <sys/stat.h>
#include <unistd.h>
namespace smb_music {
namespace {
constexpr uint64_t maxFile=1024ULL*1024*1024,maxImage=12ULL*1024*1024;
#ifndef JUKEBOX_DATA_ROOT
#define JUKEBOX_DATA_ROOT "/data/PSJukebox"
#endif
std::string lower(std::string s){
 for(char& c:s)c=(char)std::tolower((unsigned char)c);return s;
}
bool safe(const std::string& part){
 if(part.empty()||part.size()>180||part[0]=='.')return false;
 for(unsigned char c:part)if(c<32||c==127||c=='/'||c=='\\')return false;
 return true;
}
bool relative(const std::string& path){
 if(path.empty()||path.size()>510||path[0]=='/'||path.back()=='/')return false;
 size_t start=0;
 for(;;){
  size_t slash=path.find('/',start);
  if(!safe(path.substr(start,slash==std::string::npos?slash:slash-start)))return false;
  if(slash==std::string::npos)break;
  start=slash+1;
 }
 return true;
}
bool ending(const std::string& s,const std::string& tail){
 return s.size()>tail.size()&&lower(s.substr(s.size()-tail.size()))==tail;
}
struct Config{
 std::string server,share,path,username="Guest",password,domain;
 bool enabled=false;
};
std::string trim(std::string s){
 auto l=s.find_first_not_of(" \t\r\n");
 if(l==std::string::npos)return {};
 auto r=s.find_last_not_of(" \t\r\n");
 return s.substr(l,r-l+1);
}
bool config(Config& c,std::string& error){
 const std::string path=std::string(JUKEBOX_DATA_ROOT)+"/smb.ini";
 FILE* f=fopen(path.c_str(),"rb");
 if(!f){if(errno==ENOENT){error="SMB not configured (optional smb.ini)";return false;}
          error="Cannot open smb.ini";return false;}
 char row[1024];int lines=0;
 while(lines++<24&&fgets(row,sizeof(row),f)){
  std::string s=trim(row);
  if(s.empty()||s[0]=='#'||s[0]==';')continue;
  auto at=s.find('=');
  if(at==std::string::npos)continue;
  auto k=lower(trim(s.substr(0,at))),v=trim(s.substr(at+1));
  if(v.size()>255)continue;
  if(k=="server")c.server=v;
  if(k=="share")c.share=v;
  if(k=="path")c.path=v;
  if(k=="username")c.username=v;
  if(k=="password")c.password=v;
  if(k=="domain")c.domain=v;
 }
 fclose(f);
 if(c.server.empty()||c.share.empty()||c.username.empty()||
    c.server.find_first_of(" /\\\t\r\n")!=std::string::npos||
    !safe(c.share)||c.path.find('\\')!=std::string::npos||
    (c.path.size()&&c.path[0]=='/')){
  error="smb.ini needs valid server, share, path, username";return false;
 }
 if(!c.path.empty()&&!relative(c.path)){
  error="Invalid SMB subfolder in smb.ini";return false;
 }
 c.enabled=true;return true;
}
std::string actual(const Config& c,const std::string& relative){
 return c.path.empty()?relative:c.path+"/"+relative;
}
std::string virtualize(const std::string& relative){return "smbj://"+relative;}
bool decode(const std::string& path,std::string& relativePath){
 if(path.rfind("smbj://",0)!=0)return false;
 relativePath=path.substr(7);
 return relative(relativePath);
}
struct Session{
 struct smb2_context* ctx=nullptr;
 bool connected=false;
 ~Session(){
  if(ctx){
   if(connected)smb2_disconnect_share(ctx);
   smb2_destroy_context(ctx);
  }
 }
 bool open(const Config& cfg,std::string& error){
  ctx=smb2_init_context();
  if(!ctx){error="SMB memory initialization failed";return false;}
  smb2_set_user(ctx,cfg.username.c_str());
  if(!cfg.password.empty())smb2_set_password(ctx,cfg.password.c_str());
  if(!cfg.domain.empty())smb2_set_domain(ctx,cfg.domain.c_str());
  smb2_set_security_mode(ctx,SMB2_NEGOTIATE_SIGNING_ENABLED);
  smb2_set_timeout(ctx,12);
  if(smb2_connect_share(ctx,cfg.server.c_str(),cfg.share.c_str(),cfg.username.c_str())!=0){
   error="SMB connect failed (check server, share, username/password and firewall)";
   return false;
  }
  connected=true;return true;
 }
};
struct Item{std::string name;uint32_t type;uint64_t size;};
std::vector<Item> listing(Session& session,const std::string& path){
 std::vector<Item> result;
 struct smb2dir* dir=smb2_opendir(session.ctx,path.empty()?"":path.c_str());
 if(!dir)return result;
 while(auto* e=smb2_readdir(session.ctx,dir)){
  if(!safe(e->name))continue;
  if(e->st.smb2_type!=SMB2_TYPE_FILE&&e->st.smb2_type!=SMB2_TYPE_DIRECTORY)continue;
  result.push_back({e->name,e->st.smb2_type,e->st.smb2_size});
  if(result.size()>=4096)break;
 }
 smb2_closedir(session.ctx,dir);
 std::sort(result.begin(),result.end(),[](const Item&a,const Item&b){
  return lower(a.name)<lower(b.name);
 });
 return result;
}
bool transfer(Session& session,const Config& cfg,const std::string& rel,
              uint64_t size,FILE* f,std::string* data,
              std::atomic<bool>* cancel,std::atomic<uint64_t>* progress,
              std::string& error){
 struct smb2fh* fh=smb2_open(session.ctx,actual(cfg,rel).c_str(),O_RDONLY);
 if(!fh){error="SMB file cannot be opened";return false;}
 bool ok=true;uint64_t done=0;
 char block[65536];
 while(done<size){
  if(cancel&&cancel->load()){error="SMB transfer cancelled";ok=false;break;}
  const size_t want=(size_t)std::min<uint64_t>(sizeof(block),size-done);
  int got=smb2_read(session.ctx,fh,block,(uint32_t)want);
  if(got==-EAGAIN)continue;
  if(got<=0){ok=false;error="SMB file transfer interrupted";break;}
  if(f&&fwrite(block,1,(size_t)got,f)!=(size_t)got){ok=false;error="SMB cache write failed";break;}
  if(data)data->append(block,(size_t)got);
  done+=(uint64_t)got;
  if(progress)progress->store(done);
 }
 smb2_close(session.ctx,fh);
 return ok&&done==size;
}
bool artwork_file(const std::string& filename){auto n=lower(filename);
 return n=="cover.jpg"||n=="cover.jpeg"||n=="cover.png"||
        n=="background.jpg"||n=="background.jpeg"||n=="background.png"||
        n=="logo.jpg"||n=="logo.jpeg"||n=="logo.png";
}
std::string choose_art(const std::vector<Item>& files,const std::string& basename,
                       const std::string& folder){
 const std::string suffixes[]={".png",".jpg",".jpeg"};
 for(const auto& ext:suffixes){
  for(const auto& item:files)
   if(item.type==SMB2_TYPE_FILE&&item.size>8&&item.size<=maxImage&&
      lower(item.name)==basename+ext)
    return virtualize(folder+"/"+item.name);
 }
 return {};
}
uint64_t hash(const std::string& path){
 uint64_t h=14695981039346656037ULL;
 for(unsigned char c:path){h^=c;h*=1099511628211ULL;}return h;
}
}
bool is_smb(const std::string& path){std::string rel;return decode(path,rel);}
ScanResult scan(){
 ScanResult result;Config cfg;std::string error;
 if(!config(cfg,error)){result.status=error;return result;}
 Session session;
 if(!session.open(cfg,error)){result.status=error;return result;}
 auto roots=listing(session,cfg.path);
 unsigned files=0;
 for(const auto& entry:roots){
  if(entry.type==SMB2_TYPE_FILE&&ending(entry.name,".album")&&
     entry.size>=22&&entry.size<=maxFile)
   result.archives.push_back({"",virtualize(entry.name),entry.size});
  if(entry.type!=SMB2_TYPE_DIRECTORY)continue;
  if(result.library.artists.size()>=256)break;
  jukebox::Artist artist;artist.name=entry.name;artist.path=virtualize(entry.name);
  const auto contents=listing(session,actual(cfg,entry.name));
  artist.logo=choose_art(contents,"logo",entry.name);
  artist.background=choose_art(contents,"background",entry.name);
  for(const auto& item:contents){
   if(item.type==SMB2_TYPE_FILE&&lower(item.name)=="theme.ini"&&item.size>0&&item.size<=4096){
    std::string ini;ini.reserve((size_t)item.size);
    std::string readError;
    if(transfer(session,cfg,entry.name+"/"+item.name,item.size,nullptr,&ini,nullptr,nullptr,readError))
      artist.theme=jukebox::parse_theme_text(ini);
   }
   if(item.type==SMB2_TYPE_FILE&&ending(item.name,".album")&&item.size>=22&&item.size<=maxFile)
    result.archives.push_back({entry.name,virtualize(entry.name+"/"+item.name),item.size});
  }
  // A regular album and its tracks are represented as SMB pseudo-paths
  // until the user selects Play. No entire-share predownload during scan.
  auto enumerate_tracks=[&](jukebox::Album& album,const std::vector<Item>& entries,
                            const std::string& folder){
   album.cover=choose_art(entries,"cover",folder);
   album.background=choose_art(entries,"background",folder);
   for(const auto& song:entries){
    if(song.type!=SMB2_TYPE_FILE||!jukebox::is_music(song.name)||
       song.size<1024||song.size>maxFile)continue;
    const auto dot=song.name.find_last_of('.');
    std::string title=song.name.substr(0,dot);
    for(char& ch:title)if(ch=='_')ch=' ';
    album.tracks.push_back({title,virtualize(folder+"/"+song.name),song.size});
    result.library.trackCount++;if(++files>=12000||album.tracks.size()>=400)break;
   }
  };
  jukebox::Album singles;singles.name="Singles";singles.path=virtualize(entry.name);
  enumerate_tracks(singles,contents,entry.name);
  if(!singles.tracks.empty())artist.albums.push_back(std::move(singles));
  for(const auto& item:contents){
   if(item.type!=SMB2_TYPE_DIRECTORY||artist.albums.size()>=128||files>=12000)continue;
   const std::string prefix=entry.name+"/"+item.name;
   jukebox::Album album;album.name=item.name;album.path=virtualize(prefix);
   enumerate_tracks(album,listing(session,actual(cfg,prefix)),prefix);
   artist.albums.push_back(std::move(album));
  }
  result.library.artists.push_back(std::move(artist));
  if(files>=12000)break;
 }
 result.status="SMB_CONNECTED artists="+std::to_string(result.library.artists.size())+
  " tracks="+std::to_string(result.library.trackCount)+
  " packages="+std::to_string(result.archives.size());
 result.library.report=result.status;
 return result;
}
bool artwork(const std::string& path,std::string& data,std::string& error){
 std::string rel;Config cfg;data.clear();
 if(!decode(path,rel)||!config(cfg,error))return false;
 if(!artwork_file(rel.substr(rel.find_last_of('/')+1))){error="Not an SMB image";return false;}
 Session session;if(!session.open(cfg,error))return false;
 struct smb2_stat_64 st{};
 if(smb2_stat(session.ctx,actual(cfg,rel).c_str(),&st)!=0||
    st.smb2_type!=SMB2_TYPE_FILE||st.smb2_size<8||st.smb2_size>maxImage){
  error="SMB image unavailable";return false;
 }
 data.reserve((size_t)st.smb2_size);
 return transfer(session,cfg,rel,st.smb2_size,nullptr,&data,nullptr,nullptr,error);
}
bool stage(const std::string& path,uint64_t expected,std::atomic<bool>& cancel,
           std::string& local,std::string& error,std::atomic<uint64_t>* copied){
 std::string rel;Config cfg;local.clear();if(copied)copied->store(0);
 if(!decode(path,rel)||!config(cfg,error))return false;
 const bool packed=ending(rel,".album");
 if((!packed&&!jukebox::is_music(rel))||
    expected<(packed?22:1024)||expected>maxFile){error="Invalid SMB media";return false;}
 Session session;if(!session.open(cfg,error))return false;
 smb2_stat_64 st{};
 if(smb2_stat(session.ctx,actual(cfg,rel).c_str(),&st)!=0||
    st.smb2_type!=SMB2_TYPE_FILE||st.smb2_size!=expected){
  error="SMB music missing or size changed, rescan";return false;
 }
 const std::string dir=std::string(JUKEBOX_DATA_ROOT)+"/Cache";
 mkdir(JUKEBOX_DATA_ROOT,0755);mkdir(dir.c_str(),0755);
 char id[32];snprintf(id,sizeof(id),"%016llx",(unsigned long long)hash(path));
 local=dir+"/"+id+rel.substr(rel.find_last_of('.'));
 struct stat cached{};
 if(lstat(local.c_str(),&cached)==0&&S_ISREG(cached.st_mode)&&
    (uint64_t)cached.st_size==expected)return true;
 const auto part=local+".part";FILE* f=fopen(part.c_str(),"wb");
 if(!f){error="Cannot write SMB music cache";return false;}
 const bool ok=transfer(session,cfg,rel,expected,f,nullptr,&cancel,copied,error);
 if(fflush(f)!=0)error="SMB cache flush failed";
 if(fclose(f)!=0)error="SMB cache close failed";
 if(!ok||!error.empty()){remove(part.c_str());return false;}
 if(rename(part.c_str(),local.c_str())!=0){
  remove(part.c_str());error="Cannot finalize SMB music";return false;
 }
 return true;
}
}
