// SPDX-License-Identifier: GPL-3.0-or-later
// PSJukebox by YAHBALLON v0.1.18
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "library.hpp"
#include "audio_player.hpp"
#include "remote_bridge.hpp"
#include "stick_nav.hpp"
#include "cover_motion.hpp"
#include "cover_browse.hpp"
#include "hud_layout.hpp"
#include "song_navigation.hpp"
#include "favorites.hpp"
#include "album_archive.hpp"
#include "library_cache.hpp"
#include "utf8_text.hpp"
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
constexpr const char* version="v0.1.18";
const Color white=Color::rgb(0xffffff),muted=Color::rgb(0xc5c8cf),black=Color::rgb(0x000000);
Color color(uint32_t hex,float alpha=1.f){return Color::rgb(hex,alpha);}
std::string cut(const std::string& s,size_t n){return jukebox::utf8_cut(s,n);}
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

struct ScanReady {jukebox::Library library;bool complete=false;};
std::mutex scanMutex;std::vector<ScanReady> finishedScans;
std::atomic<bool> scanBusy{false};
void* scan_worker(void*){
 auto local=jukebox::scan(jukebox::music_roots());
 auto remote=jbx::scan();
 auto network=smb_music::scan();
 const std::string remoteStatus=remote.status,networkStatus=network.status;
 auto packs=jukebox::find_packed_albums(jukebox::music_roots());
 for(const auto& p:remote.archives)packs.push_back(p);
 for(const auto& p:network.archives)packs.push_back(p);
 log("ALBUM_SCAN local_or_helper="+std::to_string(packs.size())+
     " helper="+std::to_string(remote.archives.size())+
     " smb="+std::to_string(network.archives.size())+
     " helper_status="+remoteStatus);
 // Publish directory/SMB catalog immediately: do not make the UI wait for
 // packed archive transfers and ZIP indexing.
 jukebox::merge(local,std::move(remote.library));
 jukebox::merge(local,std::move(network.library));
 auto publish=[&](bool complete){
  std::lock_guard<std::mutex> lk(scanMutex);
  // Keep only the latest snapshot to avoid a frame-by-frame backlog.
  finishedScans.clear();
  finishedScans.push_back({local,complete});
 };
 if(!local.artists.empty())publish(false);
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
  jukebox::Library entry;
  if(!jukebox::unpack_album(path,artist,name,pack.path,jukebox::archive_cache_root(),musicRoot,error,&entry)){
   log("ALBUM_INVALID "+name+": "+error);
   importedStatus+="ALBUM_INVALID "+name+": "+error+"; ";continue;
  }
  const auto songs=entry.trackCount;
  if(songs==0){log("ALBUM_EMPTY_INDEX "+artist+"/"+name);continue;}
  log("ALBUM_INDEX_OK "+artist+"/"+name+" tracks="+std::to_string(songs));
  jukebox::merge(local,std::move(entry));
  publish(false); // display the next cover immediately, other .albums continue
  log("ALBUM_OK "+artist+"/"+name+" tracks="+std::to_string(songs));
  importedStatus+="ALBUM_OK "+artist+"/"+name+" tracks="+std::to_string(songs)+"; ";
 }
 local.report+=" | "+remoteStatus+" | "+networkStatus+" | "+importedStatus;
 publish(true);
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

void hud_glass(DrawList& d, float x,float y,float w,float h,float radius=21.f,
               float opacity=.92f,bool goldBorder=false){
 d.shadow({x,y,w,h},radius,21,color(0x000000,.58f));
 d.gradient_rect({x,y,w,h},radius,color(0x0d1726,opacity),color(0x030813,opacity-.05f));
 d.bordered_rect({x,y,w,h},radius,color(0x090f1b,.15f),1.4f,
                 goldBorder?color(0xf8c841,.81f):color(0xa7c1d9,.32f));
 d.line(x+radius,y+1,x+w-radius,y+1,1,color(0xc6ecff,.15f));
}
void hud_tab_icon(DrawList& d,int tab,float x,float y,Color c){
 if(tab==0){
  for(int r=0;r<2;r++)for(int col=0;col<2;col++)
   d.rounded_rect({x+col*10.f,y+r*10.f,7,7},1,c);
 }else if(tab==1){
  d.circle(x+9,y+4,5,c);d.rounded_rect({x+1,y+12,16,9},7,c);
 }else if(tab==2){
  d.line(x+8,y+3,x+18,y,2.8f,c);
  d.line(x+8,y+3,x+8,y+17,2.8f,c);
  d.line(x+18,y,x+18,y+14,2.8f,c);
  d.circle(x+5,y+18,4,c);d.circle(x+15,y+15,4,c);
 }else if(tab==3){
  d.bordered_rect({x,y+3,19,15},3,color(0x000000,0),2,c);
  for(int r=0;r<2;r++)d.line(x+5,y+8+r*6,x+15,y+8+r*6,2,c);
 }else{
  d.ring(x+5,y+7,5,2.2f,c);d.ring(x+15,y+7,5,2.2f,c);
  d.line(x+1,y+10,x+10,y+20,2.5f,c);
  d.line(x+19,y+10,x+10,y+20,2.5f,c);
 }
}
void hud_arrow(DrawList& d,float x,float y,bool right){
 d.shadow({x-28,y-28,56,56},28,10,color(0x000000,.72f));
 d.bordered_rect({x-28,y-28,56,56},28,color(0x07121e,.82f),
                 1.3f,color(0xf6cb59,.70f));
 const float dir=right?1.f:-1.f;
 d.line(x-dir*5.f,y-11,x+dir*6.f,y,3.4f,white);
 d.line(x+dir*6.f,y,x-dir*5.f,y+11,3.4f,white);
}
void hud_equalizer(DrawList& d,float x,float base,float seconds,bool paused,
                   uint32_t gold){
 for(unsigned i=0;i<jukebox::hud::equalizerBars;i++){
  const float height=jukebox::hud::visualizerBar(i,seconds,paused);
  const float bx=x+i*11.6f;
  d.glow({bx-1,base-height-1,7,height+2},4,6,color(gold,.22f));
  d.rounded_rect({bx,base-height,6,height},3,color(gold,.96f));
 }
}

void footer_chip(DrawList& d,const Font& font,unsigned atlas,float x,float y,
                 char symbol,const char* action,uint32_t tint,float width=195.f){
 d.shadow({x,y,width,57},15,9,color(0x000000,.48f));
 d.gradient_rect({x,y,width,57},15,color(0x152132,.91f),color(0x07111d,.96f));
 d.bordered_rect({x,y,width,57},15,color(0x050c16,.10f),1,color(0xabc6e3,.25f));
 d.bordered_rect({x+7,y+8,39,39},20,color(tint,.10f),2.4f,color(tint));
 const float cx=x+26,cy=y+27;
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
 label(d,font,atlas,action,x+53,y+35,18,white);
}
void footer(DrawList& d,const Font& font,unsigned atlas,int screen){
 const float y=1007;
 d.gradient_rect({0,986,1920,94},0,color(0x030711,.88f),color(0x050a13,.97f));
 d.line(38,987,1882,987,1.2f,color(0x7e9ab9,.18f));
 if(screen==0||screen==2){
  footer_chip(d,font,atlas,38,y,'L',"Artist",0xc5c7ce,165);
  footer_chip(d,font,atlas,216,y,'R',"Artist",0xc5c7ce,165);
  footer_chip(d,font,atlas,394,y,'2',"Prev Song",0xffd06a,184);
  footer_chip(d,font,atlas,591,y,'3',"Next Song",0xffd06a,184);
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
 log("BOOT v0.1.18");
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 Renderer renderer;if(!renderer.init())hui::sys::park();
 Font font;auto f=read("/app0/assets/fonts/inter-semibold.huifont",4*1024*1024);
 if(f.empty())f=read("/data/homebrew/PPSA99947/assets/fonts/inter-semibold.huifont",4*1024*1024);
 if(!font.load(f))hui::sys::park();
 unsigned atlas=renderer.batch().create_font_texture(font);
 // The original Inter font is baked for ASCII-only UI. Load a Unicode
 // fallback atlas for artist names and tracks in other writing systems.
 // Font::resolve uses the fallback before it substitutes '?'.
 Font internationalFont;
 auto intl=read("/app0/assets/fonts/international.huifont",16*1024*1024);
 if(intl.empty())intl=read("/data/homebrew/PPSA99947/assets/fonts/international.huifont",16*1024*1024);
 if(internationalFont.load(intl)){
  const unsigned internationalAtlas=renderer.batch().create_font_texture(internationalFont);
  if(internationalAtlas){
   font.add_fallback(&internationalFont,internationalAtlas);
   log("FONT_INTERNATIONAL_OK bytes="+std::to_string(intl.size()));
  }else log("FONT_INTERNATIONAL_ATLAS_FAILED");
 }else log("FONT_INTERNATIONAL_LOAD_FAILED "+internationalFont.error());
 Texture brandIcon=load_texture(renderer,"/app0/assets/icon0.png");
 if(!brandIcon.id)brandIcon=load_texture(renderer,"/data/homebrew/PPSA99947/assets/icon0.png");
 hui::ps5::Pad pad;pad.open();
 jukebox::Library library;
 std::string libraryCacheError;
 bool cacheLoaded=jukebox::load_library_cache(jukebox::library_cache_path(),
                                              library,libraryCacheError);
 if(cacheLoaded)log("LIBRARY_CACHE_HIT artists="+std::to_string(library.artists.size())+
                    " tracks="+std::to_string(library.trackCount));
 else log("LIBRARY_CACHE_MISS "+libraryCacheError);
 begin_scan();log("SCAN_START async refresh, library cache="+
                   std::string(cacheLoaded?"HIT":"MISS"));
 audio::Player player;
 jukebox::Favorites favorites;
 std::string favoritesError;
 if(!favorites.load(favoritesError))log("FAVORITES_LOAD_ERROR "+favoritesError);
 std::map<std::string,Texture> images;
 std::string playingPath,message=cacheLoaded?
   "Library ready - checking for new albums...":
   "Scanning music (covers appear as they are found)...";
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
 jukebox::CoverStickEdges coverStick;
 bool waitForListStickNeutral=false;
 int settingsReturnScreen=0;
 int pendingTrackScreen=-1; // restore Tracklist/carousel after an async USB/SMB skip
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
  // Refresh in the background WITHOUT wiping the cached covers, queued
  // music or the active decoder. A new library is swapped in when ready.
  if(begin_scan()){message="Refreshing music in background...";log("SCAN_BEGIN");}
  else {message="Cannot launch music scanner";log("SCAN_THREAD_FAIL");}
 };
 auto play_track=[&]()->bool{
  auto* a=currentAlbum();if(!a||a->tracks.empty())return false;
  trackIndex=std::clamp(trackIndex,0,(int)a->tracks.size()-1);
  const auto& t=a->tracks[(size_t)trackIndex];
  const SongLocation nextSong={static_cast<size_t>(artistIndex),
                               static_cast<size_t>(albumIndex),
                               static_cast<size_t>(trackIndex)};
  pendingTrackScreen=-1;
  if(jbx::is_remote(t.path)||smb_music::is_smb(t.path)){
   if(begin_transfer(t.path,t.size)){
    queuedSong=nextSong;
    message="Loading "+cut(t.name,45)+" from "+(smb_music::is_smb(t.path)?"SMB":"USB")+"...";
    screen=2;
    log("TRANSFER_BEGIN "+t.name+" bytes="+std::to_string(t.size));
    return true;
   }
   message="Song transfer already running or unavailable";
   return false;
  }
  if(player.start(t.path)){
   queuedSong=nextSong;playingPath=t.path;playingSong=queuedSong;
   havePlayingSong=true;screen=2;message="";
   return true;
  }
  message="Audio file inaccessible: "+cut(t.name,36);
  log(message);
  return false;
 };
 // A trigger press acts on the ACTUALLY PLAYING track. R2 walks forward,
 // L2 walks backwards. At the first/last song the neighboring nonempty
 // album is selected, with cyclic wrap inside the same artist.
 auto next_track=[&](int delta,bool fromTrigger)->bool{
  if(transferBusy.load()){
   if(fromTrigger)message="Please wait for song loading";
   return false;
  }
  jukebox::SongIndex destination;
  bool found=false;
  if(havePlayingSong){
   const jukebox::SongIndex active={(int)playingSong.artist,
                                    (int)playingSong.album,
                                    (int)playingSong.track};
   found=jukebox::step_song(library,active,delta,destination);
  }else{
   found=jukebox::start_song(library,artistIndex,albumIndex,delta,destination);
  }
  if(!found){
   if(fromTrigger)message="No playable song in selected artist";
   return false;
  }
  const int oldArtist=artistIndex,oldAlbum=albumIndex,oldTrack=trackIndex;
  const int oldScreen=screen;
  // Auto-advance preserves an unrelated browsing selection, while
  // L2/R2 deliberately synchronize coverflow and tracklist to playback.
  const bool browsingOther=havePlayingSong&&
    (oldArtist!=(int)playingSong.artist||oldAlbum!=(int)playingSong.album);
  const auto* previous=currentAlbum();
  const std::string oldBackground=previous&&oldArtist==destination.artist?
    jukebox::album_background(library.artists[(size_t)oldArtist],*previous):std::string{};
  artistIndex=destination.artist;
  albumIndex=destination.album;
  trackIndex=destination.track;
  if(fromTrigger&&oldArtist==destination.artist&&oldAlbum!=destination.album){
   coverMotion.reset();
   previousBackground=oldBackground;
   coverMotion.begin(delta);
  }else if(oldArtist!=destination.artist||oldAlbum!=destination.album){
   coverMotion.reset();
   previousBackground.clear();
  }
  const auto& selected=library.artists[(size_t)artistIndex].albums[(size_t)albumIndex];
  if(!play_track()){
   artistIndex=oldArtist;albumIndex=oldAlbum;trackIndex=oldTrack;
   coverMotion.reset();previousBackground.clear();
   return false;
  }
  // A user-triggered change keeps the current carousel or Tracklist visible.
  // On a network download, restore this view when the transfer completes.
  const int returnScreen=(oldScreen==0||oldScreen==1||oldScreen==5||oldScreen==6)?
                         oldScreen:2;
  if(fromTrigger){
   screen=returnScreen;
   if(transferBusy.load())pendingTrackScreen=returnScreen;
  }else if(browsingOther&&(oldScreen==0||oldScreen==1||oldScreen==5)){
   artistIndex=oldArtist;albumIndex=oldAlbum;trackIndex=oldTrack;
   screen=oldScreen;
   if(transferBusy.load())pendingTrackScreen=oldScreen;
  }else{
   screen=returnScreen;
   if(transferBusy.load())pendingTrackScreen=returnScreen;
  }
  log(std::string(fromTrigger?"TRIGGER_TRACK ":"AUTO_TRACK ")+
      (delta<0?"L2 ":"R2 ")+
      library.artists[(size_t)destination.artist].name+"/"+selected.name+
      " track="+std::to_string(destination.track+1));
  return true;
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
  // Keep analog-stick seek separate from D-pad cover navigation. Never
  // synthesize D-pad left/right from stick movement: both may be used while
  // playback is active and must perform independent actions.
  int analogSeekDirection=0;
  if((screen==0||screen==2)&&!headerFocus&&analogSample.connected&&
     !(analogSample.buttons&hui::pad_bits::kIntercepted)){
   const int dir=coverStick.tick(analogSample.left_x,analogSample.left_y,
                                analogSample.right_x,analogSample.right_y);
   if(dir==-1||dir==1)analogSeekDirection=dir;
   if(dir==-2)pressed|=hui::pad_bits::kUp;
   if(dir==2){pressed|=hui::pad_bits::kDown;waitForListStickNeutral=true;}
  }else coverStick.reset();
  if(--hold<=0){
   // D-pad album changes and stick seeking are edge-triggered in coverflow.
   // Holding a direction must never race through multiple album covers.
   if(screen!=0&&screen!=2&&now&hui::pad_bits::kLeft)pressed|=hui::pad_bits::kLeft;
   if(screen!=0&&screen!=2&&now&hui::pad_bits::kRight)pressed|=hui::pad_bits::kRight;
   if(screen!=0&&screen!=2&&now&hui::pad_bits::kUp)pressed|=hui::pad_bits::kUp;
   if(screen!=0&&screen!=2&&now&hui::pad_bits::kDown)pressed|=hui::pad_bits::kDown;
   hold=11;
  }
  if(screen!=0&&screen!=2&&analogSample.connected&&
     !(analogSample.buttons&hui::pad_bits::kIntercepted)){
   const bool neutral=
      analogSample.left_x>=60&&analogSample.left_x<=195&&
      analogSample.left_y>=60&&analogSample.left_y<=195&&
      analogSample.right_x>=60&&analogSample.right_x<=195&&
      analogSample.right_y>=60&&analogSample.right_y<=195;
   if(waitForListStickNeutral){
    if(neutral)waitForListStickNeutral=false;
    analogNavigator.reset();
   }else{
    switch(analogNavigator.tick(analogSample.left_x,analogSample.left_y,
                                analogSample.right_x,analogSample.right_y)){
     case -1: pressed|=hui::pad_bits::kLeft;break;
     case 1: pressed|=hui::pad_bits::kRight;break;
     case -2: pressed|=hui::pad_bits::kUp;break;
     case 2: pressed|=hui::pad_bits::kDown;break;
     default:break;
    }
   }
  }else analogNavigator.reset();
  std::vector<ScanReady> scanned;
  {std::lock_guard<std::mutex> lk(scanMutex);scanned.swap(finishedScans);}
  for(auto& item:scanned){
   // The previous on-disk catalog already gives us a usable UI: don't let
   // early incomplete scans replace it with only some of the artists.
   if(!item.complete&&(cacheLoaded||havePlayingSong))continue;
   std::string selectedArtist,selectedAlbum;
   if(artistIndex>=0&&(size_t)artistIndex<library.artists.size()){
    const auto& a=library.artists[(size_t)artistIndex];
    selectedArtist=a.name;
    if(albumIndex>=0&&(size_t)albumIndex<a.albums.size())
     selectedAlbum=a.albums[(size_t)albumIndex].path;
   }
   auto updated=std::move(item.library);
   int nextArtist=0,nextAlbum=0;
   for(size_t a=0;a<updated.artists.size();a++){
    if(updated.artists[a].name!=selectedArtist)continue;
    nextArtist=(int)a;
    for(size_t b=0;b<updated.artists[a].albums.size();b++)
     if(updated.artists[a].albums[b].path==selectedAlbum){nextAlbum=(int)b;break;}
    break;
   }
   // Remap the decoder's currently playing path before swapping a library
   // that may have reordered albums or tracks.
   bool foundPlaying=!havePlayingSong;
   SongLocation moved{};
   if(havePlayingSong){
    for(size_t a=0;a<updated.artists.size()&&!foundPlaying;a++)
     for(size_t b=0;b<updated.artists[a].albums.size()&&!foundPlaying;b++)
      for(size_t t=0;t<updated.artists[a].albums[b].tracks.size();t++)
       if(updated.artists[a].albums[b].tracks[t].path==playingPath){
        moved={a,b,t};foundPlaying=true;break;
       }
    if(!foundPlaying){player.stop();havePlayingSong=false;playingPath.clear();}
    else playingSong=moved;
   }
   library=std::move(updated);
   artistIndex=nextArtist;albumIndex=nextAlbum;trackIndex=0;
   coverMotion.reset();previousBackground.clear();
   message=item.complete?
       ("Library updated: "+std::to_string(library.artists.size())+
        " artists / "+std::to_string(library.trackCount)+" tracks"):
       ("Found "+std::to_string(library.artists.size())+" artists / "+
        std::to_string(library.trackCount)+" tracks (scanning...)");
   if(item.complete){
    cacheLoaded=true;
    if(!library.artists.empty()){
     std::string err;
     if(!jukebox::save_library_cache(jukebox::library_cache_path(),library,err))
      log("LIBRARY_CACHE_SAVE_ERROR "+err);
     else log("LIBRARY_CACHE_SAVED tracks="+std::to_string(library.trackCount));
    }
    log("SCAN_RESULT "+library.report);
   }else log("SCAN_PARTIAL artists="+std::to_string(library.artists.size())+
             " tracks="+std::to_string(library.trackCount));
  }
  std::vector<TransferDone> transfers;
  {std::lock_guard<std::mutex> lk(transferMutex);transfers.swap(finishedTransfers);}
  for(auto& item:transfers){
   if(!item.ok){message="USB transfer failed: "+item.error;log("TRANSFER_ERROR "+item.error);}
   else if(!transferCancel.load()&&player.start(item.local)){
    playingPath=item.path;playingSong=queuedSong;havePlayingSong=true;
    screen=pendingTrackScreen>=0?pendingTrackScreen:2;
    pendingTrackScreen=-1;
    message="";
    log("TRANSFER_OK "+item.path);
   }else {
    pendingTrackScreen=-1;
    message="Cannot play cached audio";log("PLAYBACK_ERROR "+item.path);
   }
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
  // Tabs use UP + LEFT/RIGHT + X. L1/R1 browse artists; R2/L2 navigate
  // tracks across neighboring albums. D-pad LEFT/RIGHT browses covers.
  // Square pauses; OPTIONS is the only settings shortcut.
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
  // R2/L2 are playback controls, not cover controls. Process them before
  // the per-screen list/carousel handlers so they work while viewing tracks.
  if(!exitDialog&&!headerFocus&&
     (screen==0||screen==1||screen==2||screen==5||screen==6)){
   const int trackStep=(pressed&hui::pad_bits::kL2)?-1:
                       (pressed&hui::pad_bits::kR2)?1:0;
   if(trackStep)next_track(trackStep,true);
   pressed&=~(hui::pad_bits::kL2|hui::pad_bits::kR2);
  }
  // L1/R1 still browse artists; D-pad left/right still browses album covers.
  if(!exitDialog&&!headerFocus&&jukebox::allows_live_album_browse(screen)){
   const int artistStep=(pressed&hui::pad_bits::kL1)?-1:
                        (pressed&hui::pad_bits::kR1)?1:0;
   const bool coverView=screen==0||screen==2;
   const int albumStep=jukebox::album_browse_step(
       coverView&&(pressed&hui::pad_bits::kLeft)!=0,
       coverView&&(pressed&hui::pad_bits::kRight)!=0);
   if(artistStep&&!library.artists.empty()){
    artistIndex=(artistIndex+artistStep+(int)library.artists.size())%
                (int)library.artists.size();
    albumIndex=trackIndex=0;
    coverMotion.reset();previousBackground.clear();
    screen=0;headerIndex=0;
    log("BROWSE_ARTIST "+library.artists[(size_t)artistIndex].name);
   }else if(albumStep){
    auto* current=currentArtist();
    auto* selected=currentAlbum();
    if(current&&current->albums.size()>1&&coverMotion.begin(albumStep)){
     previousBackground=selected?jukebox::album_background(*current,*selected):std::string{};
     albumIndex=(albumIndex+albumStep+(int)current->albums.size())%
                 (int)current->albums.size();
     trackIndex=0;
     log("BROWSE_ALBUM "+current->albums[(size_t)albumIndex].name);
    }
    screen=0;headerIndex=0;
   }
   pressed&=~(hui::pad_bits::kL1|hui::pad_bits::kR1);
   // The sticks SEEK; the D-pad CHANGES COVERS. Never mix the two inputs.
   // One seek per stick deflection, with a neutral return required to repeat.
   if(havePlayingSong&&analogSeekDirection)player.skip(10*analogSeekDirection);
   // D-pad left/right was consumed as an album-selection action in coverflow.
   if(coverView)pressed&=~(hui::pad_bits::kLeft|hui::pad_bits::kRight);
   // Down opens the selected album's tracklist on both views, even if a
   // song is already playing. Focus the playing track when it is this album.
   if((screen==0||screen==2)&&(pressed&hui::pad_bits::kDown)){
    auto* a=currentAlbum();
    if(a){
     trackIndex=(havePlayingSong&&playingSong.artist==(size_t)artistIndex&&
                 playingSong.album==(size_t)albumIndex)?
                 (int)playingSong.track:0;
     screen=1;headerIndex=2;headerFocus=false;
     analogNavigator.reset();
     log("TRACKLIST_OPEN "+a->name);
    }else message="Select an album to view its tracks";
    pressed&=~hui::pad_bits::kDown;
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
     destroy(brandIcon);pad.close();renderer.release();display.close();hui::sys::quit();
    }
    exitDialog=false;
   }
  }else if(pressed&hui::pad_bits::kCircle){
   if(screen==0){exitDialog=true;exitChoice=false;}
   else if(screen==4)screen=settingsReturnScreen;
   else if(screen==1&&havePlayingSong){screen=2;headerIndex=0;}
   else activate_tab(0);
  }else if(screen==0){
   // Album and artist selection was handled above, including when playing.
   // Refresh pointers so X always plays the newly selected album.
   artist=currentArtist();album=currentAlbum();
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
   if(pressed&hui::pad_bits::kCross){
    if(album){
     trackIndex=(havePlayingSong&&playingSong.artist==(size_t)artistIndex&&
                 playingSong.album==(size_t)albumIndex)?
                 (int)playingSong.track:0;
     screen=1;headerIndex=2;
    }
   }
   if(pressed&hui::pad_bits::kSquare)player.toggle_pause();
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
  const uint32_t accent=0xffc735; // screenshot-matched gold UI accent
  const uint32_t secondary=artist?artist->theme.secondary:0x173f37;
  const uint32_t lettering=artist?artist->theme.text:0xffffff;
  auto cached=[&](const std::string& path)->Texture {
   if(path.empty())return {};
   auto it=images.find(path);return it==images.end()?Texture{}:it->second;
  };
  auto queue=[&](const std::string& path){
   if(!path.empty()&&images.find(path)==images.end()&&!imageBusy.load())request_image(path);
  };
  // First cover MUST load before logo/background or the off-screen jackets.
  // Image loader intentionally has one in-flight request at a time.
  if(album)queue(album->cover);
  if(artist)queue(artist->logo);
  if(artist)queue(artist->background);
  if(album)queue(jukebox::album_background(*artist,*album));
  if(coverMotion.active()&&!previousBackground.empty())queue(previousBackground);
  if(havePlayingSong&&playingSong.artist<library.artists.size()){
   const auto& activeArtist=library.artists[playingSong.artist];
   if(playingSong.album<activeArtist.albums.size())queue(activeArtist.albums[playingSong.album].cover);
  }
  if(artist&&album){
   const int total=(int)artist->albums.size();
   for(const int delta:{-1,1,-2,2,-3,3}){
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
  // The screenshot uses amber/gold chrome irrespective of the artist's
  // cover palette. Artist-defined art and background remain dynamic.
  const uint32_t gold=0xffc735;
  d.gradient_rect({0,0,1920,1080},0,color(0x071c2f),color(0x020710));
  const std::string scene=artist&&!artist->background.empty()?
     artist->background:(artist&&album?jukebox::album_background(*artist,*album):std::string{});
  if(artist&&album&&coverMotion.active()&&!previousBackground.empty())
   image_fill(d,cached(previousBackground),0,0,1920,1080,.5f);
  if(!scene.empty())image_fill(d,cached(scene),0,0,1920,1080,.77f);
  // Cinematic but translucent masking: dark readable glass over existing
  // user-supplied artist scene. No hard-coded artist photos or copyrighted art.
  d.gradient_rect({0,0,1920,1080},0,color(0x02101e,.33f),color(0x03070d,.58f));
  d.gradient_rect({0,0,1920,325},0,color(0x010710,.65f),color(0x010b14,.12f));
  d.gradient_rect({0,735,1920,345},0,color(0x050910,.10f),color(0x030610,.90f));
  d.glow({45,290,36,500},16,80,color(gold,.15f));
  d.glow({1840,290,36,500},16,80,color(gold,.12f));

  // TOP HEADER — left brand lockup, glass tab rail, right live status / clock.
  const auto head=jukebox::hud::header;
  hud_glass(d,head.x,head.y,head.w,head.h,25.f,.90f);
  if(brandIcon.id){
   d.shadow({60,47,105,105},15,12,color(0x000000,.6f));
   image_fit(d,brandIcon,59,46,106,106);
   d.bordered_rect({59,46,106,106},14,color(0x000000,0),1.5f,color(0xeac9ff,.65f));
  }else{
   d.bordered_rect({60,48,100,100},17,color(gold,.11f),2,color(gold));
   d.ring(110,98,26,5,color(gold));
  }
  label(d,font,atlas,"PSJUKE",181,100,43,white);
  label(d,font,atlas,"BOX",357,100,43,color(gold));
  label(d,font,atlas,"Y O U R   M U S I C   /   Y O U R   C O N S O L E",182,131,13,muted);

  const auto tabRail=jukebox::hud::tabs;
  d.bordered_rect({tabRail.x,tabRail.y,tabRail.w,tabRail.h},22,
                  color(0x020816,.73f),1.2f,color(0x7190ac,.35f));
  static constexpr const char* tabNames[]={"All Albums","Artists","Tracks","Playlists","Favorites"};
  static constexpr float tabX[]={505.f,707.f,878.f,1048.f,1220.f};
  static constexpr float tabW[]={198.f,169.f,169.f,169.f,198.f};
  for(int i=0;i<5;i++){
   const float x=tabX[i],w=tabW[i];
   const bool selected=headerIndex==i;
   if(selected){
    d.glow({x,62,w,72},16,17,color(gold,.34f));
    d.gradient_rect({x,62,w,72},17,color(0x71521d,.92f),color(0x1c1a15,.94f));
    d.bordered_rect({x,62,w,72},17,color(0x39240d,.25f),2,color(gold));
    d.rounded_rect({x+w*.33f,131,w*.34f,4},2,color(gold));
   }
   if(selected&&headerFocus)
    d.bordered_rect({x-3,59,w+6,77},18,color(0x000000,0),2,white);
   const Color ink=selected?color(gold):color(0xc7d4e9);
   hud_tab_icon(d,i,x+29,84,ink);
   label(d,font,atlas,tabNames[i],x+59,109,18,selected?white:muted);
  }

  const auto statusBox=jukebox::hud::status;
  hud_glass(d,statusBox.x,statusBox.y,statusBox.w,statusBox.h,18.f,.96f);
  d.circle(1566,78,7,color(scanBusy.load()?0xf4be4a:0x14ee94));
  label(d,font,atlas,scanBusy.load()?"INDEXING":"LIBRARY READY",1585,84,16,white);
  const std::time_t nowTime=std::time(nullptr);
  char clockText[32]="--:--";
  if(nowTime>0){
   const auto* tm=std::localtime(&nowTime);
   if(tm)std::strftime(clockText,sizeof(clockText),"%H:%M",tm);
  }
  label(d,font,atlas,clockText,1859,91,32,white,Align::right);
  label(d,font,atlas,std::string("by YAHBALLON  ·  ")+version,
        1858,126,17,color(0xd7dce9),Align::right);

  // HERO — distinct artist heading and live indexed-library statistics.
  const auto hero=jukebox::hud::artistHero;
  hud_glass(d,hero.x,hero.y,hero.w,hero.h,20.f,.75f);
  d.glow({68,198,7,86},5,12,color(gold,.35f));
  d.rounded_rect({68,198,7,86},4,color(gold));
  label(d,font,atlas,"B R O W S I N G   A R T I S T",115,223,16,color(0xe9cc8a));
  label(d,font,atlas,artist?cut(artist->name,30):"Music Library",
        114,279,52,white);
  d.line(1420,204,1420,287,1.4f,color(0xd8e5f1,.25f));
  d.line(1563,204,1563,287,1.4f,color(0xd8e5f1,.30f));
  d.line(1690,204,1690,287,1.4f,color(0xd8e5f1,.30f));
  label(d,font,atlas,std::to_string(library.artists.size()),1494,248,30,white,Align::center);
  label(d,font,atlas,"ARTISTS",1494,275,15,color(0xc1cee0),Align::center);
  label(d,font,atlas,std::to_string(library.trackCount),1628,248,30,white,Align::center);
  label(d,font,atlas,"TRACKS",1628,275,15,color(0xc1cee0),Align::center);
  d.circle(1732,235,8,color(scanBusy.load()?0xffbf57:0x08f39a));
  label(d,font,atlas,scanBusy.load()?"SCANNING":"READY",1750,245,23,white);
  label(d,font,atlas,"LIBRARY STATUS",1750,273,14,muted);

  const bool showCoverflow=(screen==0||screen==2);
  if(showCoverflow&&album&&artist){
   // Side context cards — the artwork remains visible through both panels.
   const auto browse=jukebox::hud::browseCard;
   hud_glass(d,browse.x,browse.y,browse.w,browse.h,20.f,.73f);
   d.rounded_rect({browse.x+26,browse.y+20,7,91},4,color(gold));
   label(d,font,atlas,"NOW BROWSING",browse.x+49,browse.y+42,16,color(gold));
   label(d,font,atlas,cut(artist->name,17),browse.x+49,browse.y+87,27,white);
   const auto right=jukebox::hud::albumCard;
   hud_glass(d,right.x,right.y,right.w,right.h,20.f,.73f);
   label(d,font,atlas,"ALBUM",right.x+30,right.y+42,16,color(gold));
   label(d,font,atlas,std::to_string(albumIndex+1)+"  /  "+
         std::to_string(artist->albums.size()),right.x+30,right.y+90,31,white);

   // Five real covers, arranged to match the reference's centered coverflow.
   // Use actual user-provided artwork only; do not bake screenshots into UI.
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
    const auto slot=jukebox::cover_slot(position);
    const float x=slot.x,y=slot.y+(1.f-focus)*44.f,w=slot.w,h=slot.h;
    if(focus>.1f)d.glow({x-12,y-12,w+24,h+24},20,22,color(gold,.24f+focus*.46f));
    d.shadow({x,y,w,h},12,22,color(0x000000,.88f));
    d.bordered_rect({x,y,w,h},10,color(0x010510,.94f),
                    1.6f+3.3f*focus,focus>.5f?color(gold):color(0xc1d1e5,.83f));
    const auto cover=cached(item.cover);
    if(cover.id){
     image_fill(d,cover,x+4,y+4,w-8,h-8);
     const float reflection=jukebox::hud::reflectionHeight(y+h);
     if(reflection>3.f)
      d.image_gradient(cover.id,{x+4,y+h+5,w-8,reflection},
        {0,1,1,-.18f},color(0xffffff,.22f),color(0xffffff,0),2);
    }else{
     d.gradient_rect({x+5,y+5,w-10,h-10},1,color(secondary),color(0x050912));
     label(d,font,atlas,cut(item.name,18),x+w/2,y+h/2,24,white,Align::center);
    }
    if(focus>.6f)
     d.bordered_rect({x-2,y-2,w+4,h+4},12,color(0x000000,0),2.7f,color(gold));
   }
   if(total>1){
    hud_arrow(d,51,560,false);
    hud_arrow(d,1868,560,true);
   }
  }

  // Persistent NOW PLAYING panel — same position whether the player is
  // active or idle. Separate from browsing selection and never redundant.
  if(showCoverflow){
   const auto panel=jukebox::hud::player;
   hud_glass(d,panel.x,panel.y,panel.w,panel.h,26.f,.96f,true);
   d.glow({panel.x+27,panel.y+17,6,panel.h-34},4,16,color(gold,.26f));
   d.rounded_rect({panel.x+27,panel.y+17,7,panel.h-34},4,color(gold));
   const jukebox::Track* playingTrack=active_track();
   const jukebox::Artist* playingArtist=nullptr;
   const jukebox::Album* playingAlbum=nullptr;
   if(havePlayingSong&&playingSong.artist<library.artists.size()){
    playingArtist=&library.artists[playingSong.artist];
    if(playingSong.album<playingArtist->albums.size())
     playingAlbum=&playingArtist->albums[playingSong.album];
   }
   const auto& artbox=jukebox::hud::playerThumb;
   const std::string albumCover=playingAlbum?playingAlbum->cover:album->cover;
   d.bordered_rect({artbox.x,artbox.y,artbox.w,artbox.h},11,
                   color(0x050710,.92f),1.7f,color(0xbacbdd,.70f));
   const Texture thumb=cached(albumCover);
   if(thumb.id)image_fill(d,thumb,artbox.x+4,artbox.y+4,artbox.w-8,artbox.h-8);
   const auto playback=player.snapshot();
   label(d,font,atlas,playback.paused&&havePlayingSong?"PAUSED":"NOW PLAYING",
         375,838,16,color(gold));
   label(d,font,atlas,playingTrack?cut(playingTrack->name,50):
         "Choose an album and press X to play",375,876,30,white);
   label(d,font,atlas,playingArtist&&playingAlbum?
        cut(playingArtist->name,25)+"  /  "+cut(playingAlbum->name,34):
        (artist?cut(artist->name,32):"PSJukebox Music Library"),
        375,910,19,muted);
   const float progress=playback.duration>0&&havePlayingSong?
      std::clamp((float)(playback.seconds/playback.duration),0.f,1.f):0.f;
   d.rounded_rect({452,934,938,10},5,color(0x344459,.94f));
   if(progress>.001f)d.rounded_rect({452,934,938*progress,10},5,color(gold));
   d.glow({448+938*progress,929,18,20},9,8,color(gold,.29f));
   d.circle(452+938*progress,939,10,color(gold));
   label(d,font,atlas,stamp(playback.seconds),375,945,19,white);
   label(d,font,atlas,stamp(playback.duration),1420,945,19,white);
   hud_equalizer(d,1202,907,(float)frame*.017f,playback.paused||!havePlayingSong,gold);
   for(int i=0;i<3;i++){
    const float x=1522+i*100.f;
    d.bordered_rect({x,838,70,70},17,color(0x152132,.82f),1,color(0xc4d4e4,.30f));
    if(i==0){
     for(int bar=0;bar<4;bar++){
      const float h=12.f+((bar+1)%3)*7.f;
      d.rounded_rect({x+17.f+bar*10.f,874.f-h*.5f,5.f,h},2,color(gold));
     }
    }else if(i==1){
     d.ring(x+35,873,13,2.5f,color(0xb8cada));
     d.triangle({x+43,858,10,11},color(0xb8cada),0,1.5f);
    }else{
     for(int p=0;p<3;p++)d.circle(x+25+p*10,873,3,color(0xb8cada));
    }
   }
  }
  }else if((screen==1||screen==5)&&album){
   d.shadow({142,259,1636,670},24,24,color(0x000000,.7f));
   d.gradient_rect({142,259,1636,670},24,color(0x111b2b,.96f),color(0x080c17,.92f));
   d.bordered_rect({142,259,1636,670},24,color(0x0a121c,.12f),1.5f,color(0xffffff,.17f));
   label(d,font,atlas,screen==5?"CURRENT ALBUM QUEUE":"TRACKS  /  "+cut(album->name,30),
       205,307,33,color(accent));
   label(d,font,atlas,std::to_string(album->tracks.size())+" TRACKS",1700,305,20,muted,Align::right);
   const int start=std::max(0,trackIndex-7);
   for(int i=start;i<(int)album->tracks.size()&&i<start+10;i++){
    const auto& t=album->tracks[(size_t)i];
    const float y=370+(i-start)*52.f;
    if(i==trackIndex){
     d.glow({190,y-32,1530,48},10,9,color(accent,.16f));
     d.bordered_rect({190,y-32,1530,48},10,color(accent,.19f),1,color(accent,.55f));
    }else if((i-start)%2==0)d.rounded_rect({190,y-32,1530,48},10,color(0xffffff,.035f));
    label(d,font,atlas,std::to_string(i+1),222,y,20,color(accent));
    label(d,font,atlas,cut(t.name,86),290,y,23,white);
    if(favorites.contains(t.path)){
     d.star(1651,y-7,12,color(accent));
    }
   }
   if(album->tracks.empty())label(d,font,atlas,"No tracks in this album",960,602,26,muted,Align::center);
  }else if(screen==3){
   d.shadow({270,246,1380,665},24,25,color(0x000000,.65f));
   d.gradient_rect({270,246,1380,665},24,color(0x151d2b,.95f),color(0x080c16,.96f));
   d.bordered_rect({270,246,1380,665},24,color(0x080e19,.14f),1,color(0xffffff,.17f));
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
   d.shadow({245,239,1430,687},24,25,color(0x000000,.65f));
   d.gradient_rect({245,239,1430,687},24,color(0x171e2d,.95f),color(0x080b17,.96f));
   d.bordered_rect({245,239,1430,687},24,color(0x080f1b,.12f),1,color(0xffffff,.16f));
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
   d.shadow({330,251,1260,647},24,24,color(0x000000,.65f));
   d.gradient_rect({330,251,1260,647},24,color(0x171f2e,.95f),color(0x080c17,.96f));
   d.bordered_rect({330,251,1260,647},24,color(0x080c16,.17f),1,color(0xffffff,.16f));
   label(d,font,atlas,"SETTINGS",960,318,39,color(accent),Align::center);
   const char* options[]={"Rescan music library","Back to albums"};
   for(int i=0;i<2;i++){
    if(i==settingsIndex)d.rounded_rect({497,385+i*85.f,926,65},9,color(accent,.22f));
    label(d,font,atlas,options[i],960,430+i*85.f,27,white,Align::center);
   }
   label(d,font,atlas,"Favorites: /data/PSJukebox/favorites.json",960,721,21,muted,Align::center);
   label(d,font,atlas,"L1/R1 Artist | L2/R2 Songs | D-PAD Covers | STICK Seek | DOWN Tracks",960,765,19,muted,Align::center);
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
  if(scanBusy.load())label(d,font,atlas,"Checking for new music in background...",960,933,21,color(accent),Align::center);
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
    // Continue the playlist across album boundaries, including after the
    // last song. Reuse the same tested, empty-album-safe selection logic.
    next_track(1,false);
   }
  }
 }
}
