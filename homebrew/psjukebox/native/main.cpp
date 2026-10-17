// SPDX-License-Identifier: GPL-3.0-or-later
// PSJukebox by YAHBALLON v0.1.8
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "library.hpp"
#include "audio_player.hpp"
#include "remote_bridge.hpp"
#include "stick_nav.hpp"
#include "cover_motion.hpp"
#include "favorites.hpp"
#include "album_archive.hpp"
#include "smb_music.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "stb_image.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <map>
#include <mutex>
#include <pthread.h>
#include <string>
#include <vector>
#include <sys/stat.h>

using namespace hui::gfx;
namespace {
constexpr const char* version="v0.1.8";
const Color white=Color::rgb(0xffffff),muted=Color::rgb(0xc5c8cf),black=Color::rgb(0x000000);
Color color(uint32_t hex,float alpha=1.f){return Color::rgb(hex,alpha);}
std::string cut(std::string s,size_t n){if(s.size()>n){s.resize(n-3);s+="...";}return s;}
std::string read(const std::string& path,size_t limit=12*1024*1024){
 FILE* f=fopen(path.c_str(),"rb");if(!f)return {};
 if(fseek(f,0,SEEK_END)){fclose(f);return {};}
 const long n=ftell(f);
 if(n<=0||(size_t)n>limit){fclose(f);return {};}
 rewind(f);
 std::string bytes((size_t)n,0);
 if(fread(bytes.data(),1,bytes.size(),f)!=bytes.size())bytes.clear();
 fclose(f);return bytes;
}
struct Texture{unsigned id=0;int w=0,h=0;};
void destroy(Texture& t){if(t.id){GLuint id=t.id;glDeleteTextures(1,&id);t.id=0;}}
Texture load_texture(Renderer& renderer,const std::string& path){
 Texture result;const auto bytes=read(path);
 if(bytes.empty())return result;
 int w=0,h=0,channels=0;
 if(!stbi_info_from_memory((const stbi_uc*)bytes.data(),(int)bytes.size(),&w,&h,&channels)||
    w<1||h<1||w>4096||h>4096||(int64_t)w*h>9500000)return result;
 auto* rgba=stbi_load_from_memory((const stbi_uc*)bytes.data(),(int)bytes.size(),&w,&h,&channels,4);
 if(!rgba)return result;
 result.id=renderer.batch().create_texture(w,h,rgba);
 result.w=w;result.h=h;stbi_image_free(rgba);return result;
}
void image_fill(DrawList& d,const Texture& t,float x,float y,float w,float h,float opacity=1.f){
 if(!t.id||!t.w||!t.h)return;
 const float src=(float)t.w/t.h,dst=w/h;
 float u=0,v=0,uw=1,vh=1;
 if(src>dst){uw=dst/src;u=(1-uw)/2;}else{vh=src/dst;v=(1-vh)/2;}
 d.image(t.id,{x,y,w,h},{u,v,uw,vh},Color::rgb(0xffffff,opacity),0);
}
void image_fit(DrawList& d,const Texture& t,float x,float y,float w,float h){
 if(!t.id||!t.w||!t.h)return;
 const float scale=std::min(w/t.w,h/t.h);
 const float cw=t.w*scale,ch=t.h*scale;
 d.image(t.id,{x+(w-cw)/2,y+(h-ch)/2,cw,ch},{0,0,1,1},white,0);
}
void label(DrawList& d,const Font& font,unsigned atlas,const std::string& s,float x,float y,float sz,Color c=white,Align a=Align::left){
 d.text(font,atlas,s,x,y,sz,c,a);
}
std::string stamp(double sec){int n=(int)std::max(0.,sec);char buf[40];snprintf(buf,sizeof(buf),"%02d:%02d",n/60,n%60);return buf;}
void log(const std::string& s){
 mkdir("/data/PSJukebox",0755);
 FILE* f=fopen("/data/PSJukebox/psjukebox.log","ab");
 if(f){fprintf(f,"[PSJukebox] %s\n",s.c_str());fclose(f);}
}
struct ImageRequest {std::string path;};
struct ImageReady {std::string path,bytes;};
std::mutex imageMutex;std::vector<ImageReady> imageDone;
std::atomic<bool> imageBusy{false};
void* image_worker(void* arg){
 ImageRequest* t=static_cast<ImageRequest*>(arg);
 std::string bytes;
 if(jbx::is_remote(t->path)){std::string error;jbx::image_bytes(t->path,bytes,error);}
 else if(smb_music::is_smb(t->path)){std::string error;smb_music::artwork(t->path,bytes,error);}
 else bytes=read(t->path);
 {std::lock_guard<std::mutex> lk(imageMutex);imageDone.push_back({t->path,std::move(bytes)});}
 delete t;imageBusy.store(false);return nullptr;
}
void request_image(const std::string& path){
 if(path.empty()||imageBusy.exchange(true))return;
 auto* request=new ImageRequest{path};
 pthread_t thread{};
 if(pthread_create(&thread,nullptr,image_worker,request)!=0){delete request;imageBusy.store(false);return;}
 pthread_detach(thread);
}
Texture decode(Renderer& renderer,const std::string& bytes){
 Texture result;if(bytes.empty())return result;
 int w=0,h=0,n=0;
 if(!stbi_info_from_memory((const stbi_uc*)bytes.data(),(int)bytes.size(),&w,&h,&n)||
    w<1||h<1||w>4096||h>4096||(int64_t)w*h>9500000)return result;
 auto* rgba=stbi_load_from_memory((const stbi_uc*)bytes.data(),(int)bytes.size(),&w,&h,&n,4);
 if(!rgba)return result;
 result.id=renderer.batch().create_texture(w,h,rgba);
 result.w=w;result.h=h;stbi_image_free(rgba);return result;
}

struct ScanReady {jbx::CatalogResult remote;jukebox::Library local;};
std::mutex scanMutex;std::vector<ScanReady> finishedScans;
std::atomic<bool> scanBusy{false};
void* scan_worker(void*){
 auto local=jukebox::scan(jukebox::music_roots());
 auto remote=jbx::scan();
 auto network=smb_music::scan();
 auto packs=jukebox::find_packed_albums(jukebox::music_roots());
 for(const auto& p:remote.archives)packs.push_back(p);
 for(const auto& p:network.archives)packs.push_back(p);
 log("ALBUM_SCAN local_or_helper="+std::to_string(packs.size())+
     " helper="+std::to_string(remote.archives.size())+
     " smb="+std::to_string(network.archives.size())+
     " helper_status="+remote.status);
 std::atomic<bool> cancelled{false};
 std::string importedStatus;
 unsigned processed=0;
 for(const auto& pack:packs){
  if(processed++>=64){importedStatus+="ALBUM_LIMIT; ";break;}
  const std::string name=jukebox::album_stem(pack.path);
  log("ALBUM_FOUND name="+name+" size="+std::to_string(pack.size)+
      " source="+(jbx::is_remote(pack.path)?"HELPER":smb_music::is_smb(pack.path)?"SMB":"LOCAL"));
  if(name.empty()){importedStatus+="ALBUM_BAD_NAME; ";log("ALBUM_BAD_NAME "+pack.path);continue;}
  std::string path=pack.path,error,musicRoot;
  if(jbx::is_remote(pack.path)||smb_music::is_smb(pack.path)){
   const bool ok=jbx::is_remote(pack.path)?
     jbx::stage_track(pack.path,pack.size,cancelled,path,error):
     smb_music::stage(pack.path,pack.size,cancelled,path,error);
   if(!ok){
    log("ALBUM_TRANSFER_ERROR "+name+": "+error);
    importedStatus+="ALBUM_TRANSFER_ERROR "+name+": "+error+"; ";continue;
   }
   log("ALBUM_TRANSFER_OK "+name+" cache="+path);
  }
  std::string artist=pack.artist;
  if(artist.empty()){
   artist=jukebox::album_artist_marker(path,error);
   if(artist.empty()){
    if(error=="Root .album requires a <Name>.artist marker"){
     // Without the optional filename marker, make the album visible instead
     // of silently hiding it or guessing the artist from a release title.
     artist="Unsorted Albums";
     log("ALBUM_ARTIST_MISSING "+name+" -> Unsorted Albums");
    }else{
     log("ALBUM_ARTIST_ERROR "+name+": "+error);
     importedStatus+="ALBUM_ARTIST_ERROR "+name+": "+error+"; ";continue;
    }
   }
  }
  log("ALBUM_EXTRACT_BEGIN "+artist+"/"+name+" archive="+path);
  if(!jukebox::unpack_album(path,artist,name,pack.path,jukebox::archive_cache_root(),musicRoot,error)){
   log("ALBUM_INVALID "+name+": "+error);
   importedStatus+="ALBUM_INVALID "+name+": "+error+"; ";continue;
  }
  auto entry=jukebox::scan({musicRoot});
  const auto songs=entry.trackCount;
  jukebox::merge(local,std::move(entry));
  log("ALBUM_OK "+artist+"/"+name+" tracks="+std::to_string(songs));
  importedStatus+="ALBUM_OK "+artist+"/"+name+" tracks="+std::to_string(songs)+"; ";
 }
 jukebox::merge(local,std::move(network.library));
 local.report+=" | "+network.status+" | "+importedStatus;
 {std::lock_guard<std::mutex> lk(scanMutex);
  finishedScans.push_back({std::move(remote),std::move(local)});}
 scanBusy.store(false);
 return nullptr;
}
bool begin_scan(){
 if(scanBusy.exchange(true))return false;
 pthread_t thread{};
 if(pthread_create(&thread,nullptr,scan_worker,nullptr)!=0){scanBusy.store(false);return false;}
 pthread_detach(thread);return true;
}
struct TransferTask {std::string path;uint64_t size;};
struct TransferDone {std::string path,local,error;bool ok=false;};
std::mutex transferMutex;
std::vector<TransferDone> finishedTransfers;
std::atomic<bool> transferBusy{false},transferCancel{false};
std::atomic<uint64_t> transferDone{0},transferTotal{0};
void* transfer_worker(void* arg){
 auto* task=static_cast<TransferTask*>(arg);
 TransferDone result;result.path=task->path;
 transferTotal.store(task->size);transferDone.store(0);
 result.ok=smb_music::is_smb(task->path)?
   smb_music::stage(task->path,task->size,transferCancel,result.local,result.error,&transferDone):
   jbx::stage_track(task->path,task->size,transferCancel,result.local,result.error,&transferDone);
 {std::lock_guard<std::mutex> lk(transferMutex);finishedTransfers.push_back(std::move(result));}
 delete task;transferBusy.store(false);return nullptr;
}
bool begin_transfer(const std::string& path,uint64_t size){
 if(transferBusy.exchange(true))return false;
 transferCancel.store(false);transferDone.store(0);transferTotal.store(size);
 auto* task=new TransferTask{path,size};pthread_t thread{};
 if(pthread_create(&thread,nullptr,transfer_worker,task)!=0){
  delete task;transferBusy.store(false);return false;
 }
 pthread_detach(thread);return true;
}
void footer_chip(DrawList& d,const Font& font,unsigned atlas,float x,float y,
                 char symbol,const char* action,uint32_t tint,float width=195.f){
 d.rounded_rect({x,y,width,48},11,color(0x191c25,.92f));
 d.bordered_rect({x+5,y+6,36,36},18,color(tint,.17f),2,color(tint));
 const float cx=x+23,cy=y+24;
 const Color ink=color(tint);
 switch(symbol){
  case 'X':
   d.line(cx-9,cy-9,cx+9,cy+9,3.3f,ink);
   d.line(cx+9,cy-9,cx-9,cy+9,3.3f,ink);break;
  case 'O':d.ring(cx,cy,10,3,ink);break;
  case 'S':d.bordered_rect({cx-10,cy-10,20,20},2,color(0x000000,0),3,ink);break;
  case 'T':d.triangle({cx-11,cy-11,22,22},ink,3.0f);break;
  case 'M':
   for(int i=0;i<3;i++)d.line(cx-10,cy-8+i*8,cx+10,cy-8+i*8,2.5f,ink);
   break;
  case 'B':
   d.line(cx-10,cy,cx+10,cy,3,ink);
   d.triangle({cx-15,cy-6,9,12},ink,0,-1.5708f);
   d.triangle({cx+6,cy-6,9,12},ink,0,1.5708f);break;
  case 'U':
   d.line(cx,cy-11,cx,cy+11,3,ink);
   d.triangle({cx-6,cy-15,12,9},ink);
   d.triangle({cx-6,cy+6,12,9},ink,0,3.14159f);break;
  case 'L':label(d,font,atlas,"L1",cx,cy+7,16,ink,Align::center);break;
  case 'R':label(d,font,atlas,"R1",cx,cy+7,16,ink,Align::center);break;
  case '2':label(d,font,atlas,"L2",cx,cy+7,16,ink,Align::center);break;
  case '3':label(d,font,atlas,"R2",cx,cy+7,16,ink,Align::center);break;
 }
 label(d,font,atlas,action,x+49,y+31,19,white);
}
void footer(DrawList& d,const Font& font,unsigned atlas,int screen){
 const float y=1015;
 if(screen==0||screen==2){
  footer_chip(d,font,atlas,38,y,'L',"Artist",0xc5c7ce,165);
  footer_chip(d,font,atlas,216,y,'R',"Artist",0xc5c7ce,165);
  footer_chip(d,font,atlas,394,y,'2',"Album -",0xffd06a,184);
  footer_chip(d,font,atlas,591,y,'3',"Album +",0xffd06a,184);
  footer_chip(d,font,atlas,788,y,'X',screen==2?"Tracklist":"Play",0x78adff,204);
  footer_chip(d,font,atlas,1004,y,'S',"Pause",0xf69abe,168);
  footer_chip(d,font,atlas,1185,y,'T',"Favorite",0x67e6a7,190);
  footer_chip(d,font,atlas,1388,y,'M',"Settings",0xbdb3fb,205);
  footer_chip(d,font,atlas,1606,y,'O',"Back",0xfd8696,180);
 }else if(screen==4){
  footer_chip(d,font,atlas,720,y,'X',"Select",0x78adff,215);
  footer_chip(d,font,atlas,950,y,'O',"Back",0xfd8696,200);
 }else{
  footer_chip(d,font,atlas,470,y,'U',"Browse",0xbdbfc7,235);
  footer_chip(d,font,atlas,719,y,'X',"Play Track",0x78adff,230);
  footer_chip(d,font,atlas,963,y,'T',"Favorite",0x67e6a7,215);
  footer_chip(d,font,atlas,1192,y,'M',"Settings",0xbdb3fb,210);
 }
}
}
int main(){
 log("BOOT v0.1.8");
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 Renderer renderer;if(!renderer.init())hui::sys::park();
 Font font;auto f=read("/app0/assets/fonts/inter-semibold.huifont",4*1024*1024);
 if(f.empty())f=read("/data/homebrew/PPSA99947/assets/fonts/inter-semibold.huifont",4*1024*1024);
 if(!font.load(f))hui::sys::park();
 unsigned atlas=renderer.batch().create_font_texture(font);
 hui::ps5::Pad pad;pad.open();
 jukebox::Library library;
 begin_scan();log("SCAN_START helper discovery, local + USB");
 audio::Player player;
 jukebox::Favorites favorites;
 std::string favoritesError;
 if(!favorites.load(favoritesError))log("FAVORITES_LOAD_ERROR "+favoritesError);
 std::map<std::string,Texture> images;
 std::string playingPath,message="Scanning music (USB helper via elfldr :9021)...";
 int artistIndex=0,albumIndex=0,trackIndex=0,screen=0,settingsIndex=0;
 int headerIndex=0,artistListIndex=0,favoritesIndex=0,playlistIndex=0;
 bool headerFocus=false;
 struct SongLocation {size_t artist=0,album=0,track=0;};
 SongLocation playingSong{},queuedSong{};
 bool havePlayingSong=false;
 auto isCurrentPlaying=[&](size_t a,size_t b,size_t t){
  return havePlayingSong&&playingSong.artist==a&&playingSong.album==b&&playingSong.track==t;
 };
 auto active_track=[&]()->const jukebox::Track*{
  if(!havePlayingSong||playingSong.artist>=library.artists.size())return nullptr;
  const auto& artist=library.artists[playingSong.artist];
  if(playingSong.album>=artist.albums.size())return nullptr;
  const auto& album=artist.albums[playingSong.album];
  if(playingSong.track>=album.tracks.size())return nullptr;
  return &album.tracks[playingSong.track];
 };
 auto current_queue=[&]()->std::vector<SongLocation>{
  std::vector<SongLocation> result;
  for(size_t a=0;a<library.artists.size();a++){
   for(size_t b=0;b<library.artists[a].albums.size();b++){
    const auto& album=library.artists[a].albums[b];
    for(size_t t=0;t<album.tracks.size();t++)
     if(favorites.contains(album.tracks[t].path))result.push_back({a,b,t});
   }
  }
  return result;
 };
 // screen 0: album carousel; 1: track list; 2: now playing; 3: artist chooser; 4: settings
 bool exitDialog=false,exitChoice=false;
 unsigned held=0,frame=0;int hold=0;
 jukebox::CoverMotion coverMotion;
 std::string previousBackground;
 bool analogCoverArmed=true;
 int settingsReturnScreen=0;
 std::chrono::steady_clock::time_point lastFrame=std::chrono::steady_clock::now();
 jukebox::StickNavigator analogNavigator;
 hui::PadSample analogSample{};
 auto currentArtist=[&]()->jukebox::Artist*{
  if(library.artists.empty())return nullptr;
  artistIndex=std::clamp(artistIndex,0,(int)library.artists.size()-1);
  return &library.artists[(size_t)artistIndex];
 };
 auto currentAlbum=[&]()->jukebox::Album*{
  auto* a=currentArtist();if(!a||a->albums.empty())return nullptr;
  albumIndex=std::clamp(albumIndex,0,(int)a->albums.size()-1);
  return &a->albums[(size_t)albumIndex];
 };
 auto rescan=[&](){
  if(scanBusy.load()){message="Music library scan already running";return;}
  if(transferBusy.load()){message="Wait until song transfer finishes";return;}
  player.stop();playingPath.clear();havePlayingSong=false;
  for(auto& it:images)destroy(it.second);images.clear();
  library={};
  artistIndex=0;albumIndex=0;trackIndex=0;screen=0;
  coverMotion.reset();previousBackground.clear();
  if(begin_scan()){message="Scanning music via read-only USB helper...";log("SCAN_BEGIN");}
  else {message="Cannot launch music scanner";log("SCAN_THREAD_FAIL");}
 };
 auto play_track=[&](){
  auto* a=currentAlbum();if(!a||a->tracks.empty())return;
  trackIndex=std::clamp(trackIndex,0,(int)a->tracks.size()-1);
  const auto& t=a->tracks[(size_t)trackIndex];
  queuedSong={static_cast<size_t>(artistIndex),static_cast<size_t>(albumIndex),static_cast<size_t>(trackIndex)};
  if(jbx::is_remote(t.path)||smb_music::is_smb(t.path)){
   if(begin_transfer(t.path,t.size)){
    message="Loading "+cut(t.name,45)+" from USB...";screen=2;
    log("TRANSFER_BEGIN "+t.name+" bytes="+std::to_string(t.size));
   }else message="Song transfer already running or unavailable";
  }else if(player.start(t.path)){playingPath=t.path;playingSong=queuedSong;havePlayingSong=true;screen=2;message="";}
  else{message="Audio file inaccessible: "+cut(t.name,36);log(message);}
 };
 auto next_track=[&](int delta){
  if(havePlayingSong){
   artistIndex=static_cast<int>(playingSong.artist);
   albumIndex=static_cast<int>(playingSong.album);
   trackIndex=static_cast<int>(playingSong.track);
  }
  auto* a=currentAlbum();if(!a||a->tracks.empty())return;
  trackIndex=(trackIndex+delta+(int)a->tracks.size())%(int)a->tracks.size();
  play_track();
 };
 auto toggle_favorite=[&](const std::string& path){
  if(path.empty()){message="Select a track first";return;}
  bool added=false;std::string error;
  if(favorites.toggle(path,added,error)){
   message=added?"Added to Favorites":"Removed from Favorites";
   log(std::string("FAVORITE ")+(added?"ADD ":"REMOVE ")+jukebox::favorite_key(path));
  }else{
   message=error;log("FAVORITES_SAVE_ERROR "+error);
  }
 };
 auto activate_tab=[&](int tab){
  headerIndex=tab;headerFocus=false;
  switch(tab){
   case 0:screen=0;break;
   case 1:screen=3;artistListIndex=artistIndex;break;
   case 2:screen=1;break;
   case 3:screen=5;playlistIndex=0;break;
   case 4:screen=6;favoritesIndex=0;break;
  }
 };
 for(;;){
  std::array<hui::PadSample,64> events{};auto n=pad.read(events);
  unsigned pressed=0,now=held;
  for(size_t i=0;i<n;i++){pressed|=events[i].buttons&~now;now=events[i].buttons;}
  held=now;
  if(n)analogSample=events[n-1];
  const auto currentFrame=std::chrono::steady_clock::now();
  const float frameDt=std::clamp(std::chrono::duration<float>(currentFrame-lastFrame).count(),0.f,0.05f);
  lastFrame=currentFrame;
  coverMotion.tick(frameDt);
  int analogCoverDirection=0;
  if(screen==0&&!headerFocus&&analogSample.connected&&
     !(analogSample.buttons&hui::pad_bits::kIntercepted)){
   const int x=analogSample.left_x<60||analogSample.right_x<60?-1:
               (analogSample.left_x>195||analogSample.right_x>195?1:0);
   if(x==0)analogCoverArmed=true;
   else if(analogCoverArmed){analogCoverDirection=x;analogCoverArmed=false;}
  }else analogCoverArmed=true;
  if(--hold<=0){
   // Never synthesize repeated left/right cover presses.
   if(screen!=0&&now&hui::pad_bits::kLeft)pressed|=hui::pad_bits::kLeft;
   if(screen!=0&&now&hui::pad_bits::kRight)pressed|=hui::pad_bits::kRight;
   if(now&hui::pad_bits::kUp)pressed|=hui::pad_bits::kUp;
   if(now&hui::pad_bits::kDown)pressed|=hui::pad_bits::kDown;
   hold=11;
  }
  if(screen!=0&&analogSample.connected&&!(analogSample.buttons&hui::pad_bits::kIntercepted)){
   switch(analogNavigator.tick(analogSample.left_x,analogSample.left_y,
                               analogSample.right_x,analogSample.right_y)){
    case -1: pressed|=hui::pad_bits::kLeft;break;
    case 1: pressed|=hui::pad_bits::kRight;break;
    case -2: pressed|=hui::pad_bits::kUp;break;
    case 2: pressed|=hui::pad_bits::kDown;break;
    default:break;
   }
  }else analogNavigator.reset();
  std::vector<ScanReady> scanned;
  {std::lock_guard<std::mutex> lk(scanMutex);scanned.swap(finishedScans);}
  for(auto& item:scanned){
   library=std::move(item.local);
   jukebox::merge(library,std::move(item.remote.library));
   library.report+=" | "+item.remote.status;
   artistIndex=albumIndex=trackIndex=0;havePlayingSong=false;playingPath.clear();
   coverMotion.reset();previousBackground.clear();
   message=library.artists.empty()?
      ("No music. "+cut(item.remote.status,94)):("Loaded "+std::to_string(library.artists.size())+
        " artists / "+std::to_string(library.trackCount)+" tracks");
   log("SCAN_RESULT "+library.report);
  }
  std::vector<TransferDone> transfers;
  {std::lock_guard<std::mutex> lk(transferMutex);transfers.swap(finishedTransfers);}
  for(auto& item:transfers){
   if(!item.ok){message="USB transfer failed: "+item.error;log("TRANSFER_ERROR "+item.error);}
   else if(!transferCancel.load()&&player.start(item.local)){
    playingPath=item.path;playingSong=queuedSong;havePlayingSong=true;screen=2;message="";
    log("TRANSFER_OK "+item.path);
   }else {message="Cannot play cached audio";log("PLAYBACK_ERROR "+item.path);}
  }
  std::vector<ImageReady> ready;
  {std::lock_guard<std::mutex> lk(imageMutex);ready.swap(imageDone);}
  for(auto& result:ready){
   if(images.find(result.path)!=images.end())continue;
   images[result.path]=decode(renderer,result.bytes);
  }
  auto* artist=currentArtist();
  auto* album=currentAlbum();
  if(transferBusy.load()&&(pressed&hui::pad_bits::kCircle)){
   transferCancel.store(true);message="Cancelling USB song transfer...";pressed=0;
  }
  // Tabs use UP + LEFT/RIGHT + X. L2/R2 are reserved for albums.
  // Square never opens settings; Options is the sole Settings shortcut.
  if(!exitDialog&&pressed&hui::pad_bits::kOptions){
   if(screen==4){screen=settingsReturnScreen;}
   else{settingsReturnScreen=screen;screen=4;}
   headerFocus=false;pressed=0;
  }
  if(!exitDialog&&screen!=2&&screen!=4){
   if(headerFocus){
    if(pressed&hui::pad_bits::kLeft)headerIndex=(headerIndex+4)%5;
    if(pressed&hui::pad_bits::kRight)headerIndex=(headerIndex+1)%5;
    if(pressed&hui::pad_bits::kCross)activate_tab(headerIndex);
    if(pressed&(hui::pad_bits::kDown|hui::pad_bits::kCircle))headerFocus=false;
    pressed=0;
   }else{
    if(screen==0&&(pressed&hui::pad_bits::kUp)){headerFocus=true;pressed=0;}
   }
  }
  auto favSongs=current_queue();
  if(exitDialog){
   if(pressed&hui::pad_bits::kLeft)exitChoice=true;
   if(pressed&hui::pad_bits::kRight)exitChoice=false;
   if(pressed&hui::pad_bits::kCircle)exitDialog=false;
   if(pressed&hui::pad_bits::kCross){
    if(exitChoice){
     transferCancel.store(true);
     if(transferBusy.load()||scanBusy.load()||imageBusy.load()){
      message="Waiting for file operations to stop...";exitDialog=false;continue;
     }
     player.stop();for(auto& it:images)destroy(it.second);
     pad.close();renderer.release();display.close();hui::sys::quit();
    }
    exitDialog=false;
   }
  }else if(pressed&hui::pad_bits::kCircle){
   if(screen==0){exitDialog=true;exitChoice=false;}
   else if(screen==4)screen=settingsReturnScreen;
   else if(screen==1&&havePlayingSong){screen=2;headerIndex=0;}
   else activate_tab(0);
  }else if(screen==0){
   if(!library.artists.empty()){
    if(pressed&hui::pad_bits::kL1){
     artistIndex=(artistIndex+(int)library.artists.size()-1)%(int)library.artists.size();
     albumIndex=trackIndex=0;coverMotion.reset();previousBackground.clear();
    }
    if(pressed&hui::pad_bits::kR1){
     artistIndex=(artistIndex+1)%(int)library.artists.size();
     albumIndex=trackIndex=0;coverMotion.reset();previousBackground.clear();
    }
   }
   // Triggers select exactly one jacket per press; while sliding, skip input.
   const int step=(pressed&hui::pad_bits::kL2)?-1:
                  (pressed&hui::pad_bits::kR2)?1:
                  (pressed&hui::pad_bits::kLeft)?-1:
                  (pressed&hui::pad_bits::kRight)?1:analogCoverDirection;
   artist=currentArtist();album=currentAlbum();
   if(artist&&artist->albums.size()>1&&step&&coverMotion.begin(step)){
    previousBackground=album?jukebox::album_background(*artist,*album):std::string{};
    const int count=(int)artist->albums.size();
    albumIndex=(albumIndex+step+count)%count;
    trackIndex=0;
   }
   if(pressed&hui::pad_bits::kDown){
    if(album){trackIndex=0;screen=1;headerIndex=2;headerFocus=false;}
    else message="Select an album to view its tracks";
   }
   if(pressed&hui::pad_bits::kCross&&album&&!album->tracks.empty()){trackIndex=0;play_track();}
   if(pressed&hui::pad_bits::kTriangle){
    if(const auto* song=active_track())toggle_favorite(song->path);
    else if(album&&!album->tracks.empty())toggle_favorite(album->tracks[0].path);
    else message="Album has no songs to favorite";
   }
   if(pressed&hui::pad_bits::kSquare){
    if(havePlayingSong)player.toggle_pause();
    else message="Select an album with X to play";
   }
  }else if(screen==1||screen==5){
   if(album&&!album->tracks.empty()){
    if(pressed&hui::pad_bits::kUp)trackIndex=std::max(0,trackIndex-1);
    if(pressed&hui::pad_bits::kDown)trackIndex=std::min((int)album->tracks.size()-1,trackIndex+1);
    if(pressed&hui::pad_bits::kCross)play_track();
    if(pressed&hui::pad_bits::kTriangle)toggle_favorite(album->tracks[(size_t)trackIndex].path);
   }
   if(pressed&hui::pad_bits::kSquare&&havePlayingSong)player.toggle_pause();
  }else if(screen==2){
   if(pressed&hui::pad_bits::kCross){screen=1;headerIndex=2;}
   if(pressed&hui::pad_bits::kSquare)player.toggle_pause();
   if(pressed&hui::pad_bits::kLeft)player.skip(-10);
   if(pressed&hui::pad_bits::kRight)player.skip(10);
   if(pressed&hui::pad_bits::kL1)next_track(-1);
   if(pressed&hui::pad_bits::kR1)next_track(1);
   if(pressed&hui::pad_bits::kTriangle){
    const auto* track=active_track();toggle_favorite(track?track->path:std::string{});
   }
  }else if(screen==3){
   if(!library.artists.empty()){
    if(pressed&hui::pad_bits::kUp)artistListIndex=std::max(0,artistListIndex-1);
    if(pressed&hui::pad_bits::kDown)artistListIndex=std::min((int)library.artists.size()-1,artistListIndex+1);
    if(pressed&hui::pad_bits::kCross){
     artistIndex=artistListIndex;albumIndex=trackIndex=0;activate_tab(0);
    }
   }
  }else if(screen==6){
   if(!favSongs.empty()){
    if(pressed&hui::pad_bits::kUp)favoritesIndex=std::max(0,favoritesIndex-1);
    if(pressed&hui::pad_bits::kDown)favoritesIndex=std::min((int)favSongs.size()-1,favoritesIndex+1);
    favoritesIndex=std::clamp(favoritesIndex,0,(int)favSongs.size()-1);
    auto loc=favSongs[(size_t)favoritesIndex];
    const auto& track=library.artists[loc.artist].albums[loc.album].tracks[loc.track];
    if(pressed&hui::pad_bits::kTriangle)toggle_favorite(track.path);
    if(pressed&hui::pad_bits::kCross){
     artistIndex=(int)loc.artist;albumIndex=(int)loc.album;trackIndex=(int)loc.track;play_track();
    }
   }
  }else if(screen==4){
   if(pressed&hui::pad_bits::kUp)settingsIndex=std::max(0,settingsIndex-1);
   if(pressed&hui::pad_bits::kDown)settingsIndex=std::min(1,settingsIndex+1);
   if(pressed&hui::pad_bits::kCross){
    if(settingsIndex==0)rescan();else screen=settingsReturnScreen;
   }
  }
  artist=currentArtist();album=currentAlbum();
  const uint32_t accent=artist?artist->theme.accent:0xffc735;
  const uint32_t secondary=artist?artist->theme.secondary:0x173f37;
  const uint32_t lettering=artist?artist->theme.text:0xffffff;
  auto cached=[&](const std::string& path)->Texture {
   if(path.empty())return {};
   auto it=images.find(path);return it==images.end()?Texture{}:it->second;
  };
  auto queue=[&](const std::string& path){
   if(!path.empty()&&images.find(path)==images.end()&&!imageBusy.load())request_image(path);
  };
  if(artist)queue(artist->logo);
  if(coverMotion.active()&&!previousBackground.empty())queue(previousBackground);
 if(havePlayingSong&&playingSong.artist<library.artists.size()){
  const auto& activeArtist=library.artists[playingSong.artist];
  if(playingSong.album<activeArtist.albums.size())queue(activeArtist.albums[playingSong.album].cover);
 }
  if(album)queue(jukebox::album_background(*artist,*album));
  if(artist&&album){
   const int total=(int)artist->albums.size();
   for(int delta=-3;delta<=3;delta++){
    if(total<5&&std::abs(delta)>total/2)continue;
    const int j=(albumIndex+delta+total)%total;
    queue(artist->albums[(size_t)j].cover);
   }
  }
  if(images.size()>24){
   for(auto it=images.begin();it!=images.end()&&images.size()>16;){
    const bool keep=(coverMotion.active()&&it->first==previousBackground)||
      (artist&&(it->first==artist->logo||it->first==artist->background))||
      (album&&(it->first==album->cover||it->first==album->background));
    if(keep){++it;continue;}destroy(it->second);it=images.erase(it);
   }
  }
  DrawList d;
  d.gradient_rect({0,0,1920,1080},0,color(secondary),color(0x030409));
  if(artist&&album){
   if(coverMotion.active()&&!previousBackground.empty())
    image_fill(d,cached(previousBackground),0,0,1920,1080);
   const auto bg=cached(jukebox::album_background(*artist,*album));
   const float p=coverMotion.progress(),blend=p*p*(3.f-2.f*p);
   image_fill(d,bg,0,0,1920,1080,coverMotion.active()?blend:1.f);
  }
  // Cinematic menu chrome inspired by the supplied concept image.
  d.gradient_rect({0,0,1920,160},0,color(0x000000,.75f),color(0x000000,.04f));
  d.rounded_rect({0,0,1920,140},0,color(0x05070e,.73f));
  d.line(0,139,1920,139,2,color(accent,.50f));
  d.rounded_rect({0,972,1920,108},0,color(0x010205,.93f));
  d.line(0,973,1920,973,1,color(0xffffff,.2f));
  if(artist){
   image_fit(d,cached(artist->logo),22,8,465,98);
   if(!cached(artist->logo).id)label(d,font,atlas,cut(artist->name,24),48,74,41,color(accent));
  }else label(d,font,atlas,"PSJUKEBOX",48,76,40,color(accent));
  // Top library tabs are controller accessible: UP then left/right + X,
  // or L2/R2 cycles when not playing.
  static constexpr const char* tabNames[]={"All Albums","Artists","Tracks","Playlists","Favorites"};
  static constexpr float tabX[]={566.f,716.f,866.f,1016.f,1166.f};
  for(int i=0;i<5;i++){
   const float x=tabX[i];
   const bool active=(i==headerIndex);
   if(active)d.glow({x-46,25,92,76},17,12,color(accent,.54f));
   if(headerFocus&&active)d.bordered_rect({x-65,15,130,108},11,color(accent,.12f),2,color(accent));
   if(i==0){
    d.ring(x+4,58,8,2.5f,active?color(accent):white);
    d.line(x+12,38,x+12,60,3,active?color(accent):white);
    d.line(x+12,38,x+24,42,2.5f,active?color(accent):white);
   }else if(i==1){
    d.circle(x,47,10,active?color(accent):white);
    d.rounded_rect({x-19,60,38,12},7,active?color(accent):white);
   }else if(i==2){
    d.ring(x+5,60,8,2.5f,active?color(accent):white);
    d.line(x+13,39,x+13,59,3,active?color(accent):white);
   }else if(i==3){
    for(int j=0;j<3;j++){
     d.circle(x-16,43+j*11,2.8f,active?color(accent):white);
     d.line(x-7,43+j*11,x+17,43+j*11,2.4f,active?color(accent):white);
    }
   }else{
    // A golden favorite star replaces USB in the supplied reference.
    d.star(x,55,17,active?color(accent):color(0xffffff),0);
   }
   label(d,font,atlas,tabNames[i],x,111,19,active?color(accent):white,Align::center);
   if(active)d.line(x-56,138,x+56,138,3,color(accent));
  }
  const std::time_t nowTime=std::time(nullptr);
  char clockText[32]="--:--";
  if(nowTime>0){
   const auto* tm=std::localtime(&nowTime);
   if(tm)std::strftime(clockText,sizeof(clockText),"%H:%M",tm);
  }
  label(d,font,atlas,clockText,1850,64,27,white,Align::right);
  if(artist){
   label(d,font,atlas,cut(artist->name,33)+" LIBRARY",78,186,24,color(accent));
   label(d,font,atlas,std::string(" /  ")+tabNames[headerIndex],420,186,22,white);
  }
  const bool showCoverflow=(screen==0||screen==2);
  if(showCoverflow&&album&&artist){
   label(d,font,atlas,cut(album->name,36),960,343,36,white,Align::center);
   label(d,font,atlas,std::to_string(albumIndex+1)+" / "+std::to_string(artist->albums.size()),1820,186,21,white,Align::right);
   // 480ms smoothstep: every sleeve moves into its neighbor's position.
   // Draw farther sleeves first so the foreground jacket stays on top.
   const int total=(int)artist->albums.size();
   std::vector<int> offsets;
   for(int delta=-2;delta<=2;delta++){
    if(total<5){
     if(total==1&&delta!=0)continue;
     if(total==2&&delta!=-1&&delta!=0)continue;
     if(total==3&&(delta==-2||delta==2))continue;
     if(total==4&&delta==-2)continue;
    }
    offsets.push_back(delta);
   }
   const float slide=coverMotion.offset();
   std::sort(offsets.begin(),offsets.end(),[&](int a,int b){
    return std::fabs((float)a+slide)>std::fabs((float)b+slide);
   });
   for(const int delta:offsets){
    const int index=(albumIndex+delta+total)%total;
    const auto& item=artist->albums[(size_t)index];
    const float position=(float)delta+slide;
    if(std::fabs(position)>2.7f)continue;
    const float focus=std::clamp(1.f-std::fabs(position),0.f,1.f);
    const float w=318.f+77.f*focus,h=320.f+96.f*focus;
    const float x=960.f+position*350.f-w*.5f,y=424.f-51.f*focus;
    if(focus>.1f)d.glow({x-8,y-8,w+16,h+16},15,9,color(accent,.5f+focus*.5f));
    d.shadow({x,y,w,h},8,17,color(0x000000,.82f));
    d.bordered_rect({x,y,w,h},5,color(0x060913),2.f+3.f*focus,
      focus>.5f?color(accent):color(0xd4d8e2));
    const auto cover=cached(item.cover);
    if(cover.id){
     image_fill(d,cover,x+4,y+4,w-8,h-8);
     const float reflectionHeight=std::min(76.f,970.f-(y+h+8));
     if(reflectionHeight>10.f)
      d.image_gradient(cover.id,{x+4,y+h+6,w-8,reflectionHeight},
        {0,1,1,-.22f},color(0xffffff,.22f),color(0xffffff,0),2);
    }else{
     d.gradient_rect({x+5,y+5,w-10,h-10},0,color(secondary),color(0x0c0e17));
     label(d,font,atlas,cut(item.name,19),x+w/2,y+h/2,24,white,Align::center);
    }
   }
   // Always show a compact mini-player below the center cover like the reference.
   if(havePlayingSong&&playingSong.artist<library.artists.size()){
    const auto& current=library.artists[playingSong.artist];
    if(playingSong.album<current.albums.size()){
     const auto& currentAlbum=current.albums[playingSong.album];
     if(playingSong.track<currentAlbum.tracks.size()){
      const auto& track=currentAlbum.tracks[playingSong.track];
      auto state=player.snapshot();
      label(d,font,atlas,cut(track.name,49),960,836,27,color(accent),Align::center);
      label(d,font,atlas,cut(current.name,24)+" - "+cut(currentAlbum.name,33),960,866,18,white,Align::center);
      const float prog=state.duration>0?std::clamp((float)(state.seconds/state.duration),0.f,1.f):0.f;
      d.rounded_rect({574,914,760,12},6,color(0x9aa2b4,.7f));
      d.rounded_rect({574,914,std::max(3.f,760*prog),12},6,color(accent));
      label(d,font,atlas,stamp(state.seconds),550,926,18,white,Align::right);
      label(d,font,atlas,stamp(state.duration),1359,926,18,white);
      label(d,font,atlas,state.paused?"PAUSED":"NOW PLAYING",960,955,16,muted,Align::center);
     }
    }
   }else{
    label(d,font,atlas,"L2/R2 Albums   DOWN Tracklist   X Play   Triangle Favorite",960,902,19,muted,Align::center);
   }
  }else if((screen==1||screen==5)&&album){
   d.rounded_rect({142,245,1636,680},13,color(0x050810,.88f));
   label(d,font,atlas,screen==5?"CURRENT ALBUM QUEUE":"TRACKS  /  "+cut(album->name,30),
       205,307,33,color(accent));
   label(d,font,atlas,std::to_string(album->tracks.size())+" TRACKS",1700,305,20,muted,Align::right);
   const int start=std::max(0,trackIndex-7);
   for(int i=start;i<(int)album->tracks.size()&&i<start+10;i++){
    const auto& t=album->tracks[(size_t)i];
    const float y=370+(i-start)*52.f;
    if(i==trackIndex)d.rounded_rect({190,y-32,1530,48},7,color(accent,.22f));
    label(d,font,atlas,std::to_string(i+1),222,y,20,color(accent));
    label(d,font,atlas,cut(t.name,86),290,y,23,white);
    if(favorites.contains(t.path)){
     d.star(1651,y-7,12,color(accent));
    }
   }
   if(album->tracks.empty())label(d,font,atlas,"No tracks in this album",960,602,26,muted,Align::center);
  }else if(screen==3){
   d.rounded_rect({270,246,1380,665},12,color(0x040811,.85f));
   label(d,font,atlas,"ARTISTS",960,315,38,color(accent),Align::center);
   for(size_t i=0;i<library.artists.size()&&i<11;i++){
    const float y=375+((int)i-std::max(0,artistListIndex-5))*53.f;
    if(y<355||y>885)continue;
    if((int)i==artistListIndex)d.rounded_rect({330,y-31,1260,47},8,color(accent,.27f));
    label(d,font,atlas,cut(library.artists[i].name,53),381,y,26,white);
    label(d,font,atlas,std::to_string(library.artists[i].albums.size())+" albums",
        1540,y,20,muted,Align::right);
   }
  }else if(screen==6){
   d.rounded_rect({245,239,1430,687},13,color(0x050711,.88f));
   label(d,font,atlas,"FAVORITES",960,310,38,color(accent),Align::center);
   label(d,font,atlas,std::to_string(favSongs.size())+" SAVED SONGS",1554,315,19,muted,Align::right);
   if(favSongs.empty())label(d,font,atlas,"No favorites yet - select a song and press Triangle",960,577,25,white,Align::center);
   const int start=std::max(0,favoritesIndex-6);
   for(int i=start;i<(int)favSongs.size()&&i<start+11;i++){
    const auto location=favSongs[(size_t)i];
    const auto& a=library.artists[location.artist];
    const auto& b=a.albums[location.album];
    const auto& track=b.tracks[location.track];
    const float y=386+(i-start)*47.f;
    if(i==favoritesIndex)d.rounded_rect({298,y-33,1295,45},8,color(accent,.26f));
    d.star(340,y-10,11,color(accent));
    label(d,font,atlas,cut(track.name,64),373,y,22,white);
    label(d,font,atlas,cut(a.name,20),1512,y,17,muted,Align::right);
   }
  }else if(screen==4){
   d.rounded_rect({330,251,1260,647},14,color(0x040711,.9f));
   label(d,font,atlas,"SETTINGS",960,318,39,color(accent),Align::center);
   const char* options[]={"Rescan music library","Back to albums"};
   for(int i=0;i<2;i++){
    if(i==settingsIndex)d.rounded_rect({497,385+i*85.f,926,65},9,color(accent,.22f));
    label(d,font,atlas,options[i],960,430+i*85.f,27,white,Align::center);
   }
   label(d,font,atlas,"Favorites: /data/PSJukebox/favorites.json",960,721,21,muted,Align::center);
   label(d,font,atlas,"L1/R1 Artist | L2/R2 Album | Down Tracklist | SMB: /data/PSJukebox/smb.ini",960,765,19,muted,Align::center);
  }else{
   label(d,font,atlas,"NO ALBUMS FOUND",960,485,34,color(accent),Align::center);
   label(d,font,atlas,"Music/Artist/Album/cover.jpg + song.mp3",960,534,23,white,Align::center);
  }
  if(transferBusy.load()){
   const uint64_t total=transferTotal.load(),done=transferDone.load();
   const float frac=total?(float)std::min(1.,(double)done/(double)total):0.f;
   d.rounded_rect({340,855,1240,110},9,color(0x050b17,.95f));
   label(d,font,atlas,"COPYING MUSIC FROM USB   "+std::to_string((int)(frac*100))+"%  (O Cancel)",960,894,21,white,Align::center);
   d.rounded_rect({424,923,1070,16},7,color(0x3b4352));
   d.rounded_rect({424,923,std::max(3.f,1070*frac),16},7,color(accent));
  }
  if(scanBusy.load())label(d,font,atlas,"Scanning local and USB music via helper...",960,933,21,color(accent),Align::center);
  if(!message.empty())label(d,font,atlas,cut(message,126),960,962,17,color(0xffa65d),Align::center);
  footer(d,font,atlas,screen);
  if(exitDialog){
   d.rounded_rect({0,0,1920,1080},0,color(0x000000,.75f));
   d.bordered_rect({550,374,820,330},13,color(0x141720),2,color(accent));
   label(d,font,atlas,"EXIT PSJUKEBOX?",960,469,40,white,Align::center);
   label(d,font,atlas,exitChoice?"[ YES ]                     NO":"YES                     [ NO ]",960,593,29,color(accent),Align::center);
   label(d,font,atlas,"LEFT/RIGHT Select     X Confirm     O Cancel",960,647,19,muted,Align::center);
  }
  renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
  if(++frame%90==0&&havePlayingSong){
   auto state=player.snapshot();
   if(state.ended){
    const auto& current=library.artists[playingSong.artist].albums[playingSong.album];
    if(playingSong.track+1<current.tracks.size())next_track(1);
   }
  }
 }
}
