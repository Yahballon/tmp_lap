// SPDX-License-Identifier: GPL-3.0-or-later
// PSJukebox by YAHBALLON v0.1.0
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "library.hpp"
#include "audio_player.hpp"
#include "remote_bridge.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "stb_image.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <map>
#include <mutex>
#include <pthread.h>
#include <string>
#include <vector>
#include <sys/stat.h>

using namespace hui::gfx;
namespace {
constexpr const char* version="v0.1.1";
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
void image_fill(DrawList& d,const Texture& t,float x,float y,float w,float h){
 if(!t.id||!t.w||!t.h)return;
 const float src=(float)t.w/t.h,dst=w/h;
 float u=0,v=0,uw=1,vh=1;
 if(src>dst){uw=dst/src;u=(1-uw)/2;}else{vh=src/dst;v=(1-vh)/2;}
 d.image(t.id,{x,y,w,h},{u,v,uw,vh},white,0);
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
 result.ok=jbx::stage_track(task->path,task->size,transferCancel,result.local,result.error,&transferDone);
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
}
int main(){
 log("BOOT v0.1.1");
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
 std::map<std::string,Texture> images;
 std::string playingPath,message="Scanning music (USB helper via elfldr :9021)...";
 int artistIndex=0,albumIndex=0,trackIndex=0,screen=0,settingsIndex=0;
 // screen 0: album carousel; 1: track list; 2: now playing; 3: artist chooser; 4: settings
 bool exitDialog=false,exitChoice=false;
 unsigned held=0,frame=0;int hold=0;
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
  player.stop();playingPath.clear();
  for(auto& it:images)destroy(it.second);images.clear();
  library={};
  artistIndex=0;albumIndex=0;trackIndex=0;screen=0;
  if(begin_scan()){message="Scanning music via read-only USB helper...";log("SCAN_BEGIN");}
  else {message="Cannot launch music scanner";log("SCAN_THREAD_FAIL");}
 };
 auto play_track=[&](){
  auto* a=currentAlbum();if(!a||a->tracks.empty())return;
  trackIndex=std::clamp(trackIndex,0,(int)a->tracks.size()-1);
  const auto& t=a->tracks[(size_t)trackIndex];
  if(jbx::is_remote(t.path)){
   if(begin_transfer(t.path,t.size)){
    message="Loading "+cut(t.name,45)+" from USB...";screen=2;
    log("TRANSFER_BEGIN "+t.name+" bytes="+std::to_string(t.size));
   }else message="Song transfer already running or unavailable";
  }else if(player.start(t.path)){playingPath=t.path;screen=2;message="";}
  else{message="Audio file inaccessible: "+cut(t.name,36);log(message);}
 };
 auto next_track=[&](int delta){
  auto* a=currentAlbum();if(!a||a->tracks.empty())return;
  trackIndex=(trackIndex+delta+(int)a->tracks.size())%(int)a->tracks.size();
  play_track();
 };
 for(;;){
  std::array<hui::PadSample,64> events{};auto n=pad.read(events);
  unsigned pressed=0,now=held;
  for(size_t i=0;i<n;i++){pressed|=events[i].buttons&~now;now=events[i].buttons;}
  held=now;
  if(--hold<=0){
   if(now&hui::pad_bits::kLeft)pressed|=hui::pad_bits::kLeft;
   if(now&hui::pad_bits::kRight)pressed|=hui::pad_bits::kRight;
   if(now&hui::pad_bits::kUp)pressed|=hui::pad_bits::kUp;
   if(now&hui::pad_bits::kDown)pressed|=hui::pad_bits::kDown;
   hold=11;
  }
  std::vector<ScanReady> scanned;
  {std::lock_guard<std::mutex> lk(scanMutex);scanned.swap(finishedScans);}
  for(auto& item:scanned){
   library=std::move(item.local);
   auto& remote=item.remote.library;
   library.trackCount+=remote.trackCount;
   for(auto& artist:remote.artists)library.artists.push_back(std::move(artist));
   library.report+=" | "+item.remote.status;
   artistIndex=albumIndex=trackIndex=0;
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
    playingPath=item.path;screen=2;message="";
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
   else if(screen==2)screen=1;
   else screen=0;
  }else if(screen==0){
   if(pressed&hui::pad_bits::kLeft)albumIndex=std::max(0,albumIndex-1);
   if(pressed&hui::pad_bits::kRight&&artist)albumIndex=std::min((int)artist->albums.size()-1,albumIndex+1);
   if(pressed&hui::pad_bits::kL1){artistIndex=std::max(0,artistIndex-1);albumIndex=0;}
   if(pressed&hui::pad_bits::kR1&&artistIndex+1<(int)library.artists.size()){artistIndex++;albumIndex=0;}
   if(pressed&hui::pad_bits::kCross&&album){screen=1;trackIndex=0;}
   if(pressed&hui::pad_bits::kTriangle&&album){screen=1;trackIndex=0;}
   if(pressed&hui::pad_bits::kSquare)rescan();
   if(pressed&hui::pad_bits::kOptions)screen=4;
  }else if(screen==1){
   if(album&&!album->tracks.empty()){
    if(pressed&hui::pad_bits::kUp)trackIndex=std::max(0,trackIndex-1);
    if(pressed&hui::pad_bits::kDown)trackIndex=std::min((int)album->tracks.size()-1,trackIndex+1);
    if(pressed&hui::pad_bits::kCross)play_track();
   }
   if(pressed&hui::pad_bits::kL1)screen=0;
   if(pressed&hui::pad_bits::kR1&&album&&!album->tracks.empty())play_track();
  }else if(screen==2){
   if(pressed&hui::pad_bits::kCross)player.toggle_pause();
   if(pressed&hui::pad_bits::kLeft)player.skip(-10);
   if(pressed&hui::pad_bits::kRight)player.skip(10);
   if(pressed&hui::pad_bits::kL1)next_track(-1);
   if(pressed&hui::pad_bits::kR1)next_track(1);
   if(pressed&hui::pad_bits::kTriangle)screen=1;
  }else if(screen==4){
   if(pressed&hui::pad_bits::kUp)settingsIndex=std::max(0,settingsIndex-1);
   if(pressed&hui::pad_bits::kDown)settingsIndex=std::min(1,settingsIndex+1);
   if(pressed&hui::pad_bits::kCross){
    if(settingsIndex==0)rescan();
    else screen=0;
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
  if(album)queue(jukebox::album_background(*artist,*album));
  if(artist&&album)for(int delta=-2;delta<=2;delta++){
   const int j=albumIndex+delta;
   if(j>=0&&j<(int)artist->albums.size())queue(artist->albums[(size_t)j].cover);
  }
  if(images.size()>24){
   for(auto it=images.begin();it!=images.end()&&images.size()>16;){
    const bool keep=artist&&(it->first==artist->logo||it->first==artist->background)||
      (album&&(it->first==album->cover||it->first==album->background));
    if(keep){++it;continue;}destroy(it->second);it=images.erase(it);
   }
  }
  DrawList d;
  d.gradient_rect({0,0,1920,1080},0,color(secondary),color(0x030409));
  if(artist&&album){
   const auto bg=cached(jukebox::album_background(*artist,*album));
   image_fill(d,bg,0,0,1920,1080);
  }
  // Fixed charcoal translucent header/footer keep themes readable.
  d.rounded_rect({0,0,1920,102},0,color(0x000000,.76f));
  d.line(0,101,1920,101,1,color(0xffffff,.15f));
  d.rounded_rect({0,998,1920,82},0,color(0x000000,.87f));
  d.line(0,998,1920,998,1,color(0xffffff,.2f));
  if(artist){
   image_fit(d,cached(artist->logo),35,10,320,73);
   if(cached(artist->logo).id==0)label(d,font,atlas,cut(artist->name,24),52,59,38,color(accent));
   label(d,font,atlas,"Multi Album Player",360,71,20,muted);
  }else label(d,font,atlas,"PSJUKEBOX",55,68,40,color(accent));
  label(d,font,atlas,"PSJUKEBOX by YAHBALLON",960,52,23,white,Align::center);
  label(d,font,atlas,version,1864,52,22,color(accent),Align::right);
  if(artist){
   label(d,font,atlas,cut(artist->name,38)+" LIBRARY",89,171,26,color(accent));
   label(d,font,atlas,"/  All Albums",510,171,23,white);
  }
  if(screen==0&&album&&artist){
   label(d,font,atlas,cut(album->name,33),960,313,34,white,Align::center);
   label(d,font,atlas,std::to_string(albumIndex+1)+" / "+std::to_string(artist->albums.size()),1834,167,21,white,Align::right);
   for(int delta=-2;delta<=2;delta++){
    const int i=albumIndex+delta;
    if(i<0||i>=(int)artist->albums.size())continue;
    const auto& item=artist->albums[(size_t)i];
    const bool selected=delta==0;
    const float w=selected?386.f:302.f,h=selected?438.f:330.f;
    const float x=960+delta*340.f-w/2,y=selected?335.f:390.f;
    if(selected)d.glow({x-8,y-8,w+16,h+16},15,9,color(accent));
    d.shadow({x,y,w,h},10,15,color(0x000000,.82f));
    d.bordered_rect({x,y,w,h},7,color(0x0a0b12),selected?4.f:2.f,selected?color(accent):color(0xffffff,.82f));
    auto cover=cached(item.cover);
    if(cover.id)image_fill(d,cover,x+5,y+5,w-10,h-10);
    else{
     d.gradient_rect({x+5,y+5,w-10,h-10},0,color(secondary),color(0x0a0a12));
     label(d,font,atlas,cut(item.name,18),x+w/2,y+h/2,24,white,Align::center);
    }
    if(selected)label(d,font,atlas,cut(item.name,30),960,804,24,color(accent),Align::center);
   }
   label(d,font,atlas,"USB / LOCAL MUSIC",960,910,18,white,Align::center);
   label(d,font,atlas,"LEFT/RIGHT Browse     L1/R1 Artist     X Tracks     TRIANGLE Details     SQUARE Rescan     OPTIONS Settings     O Quit",960,1050,19,white,Align::center);
  }else if(screen==1&&album){
   d.rounded_rect({110,230,1700,696},12,color(0x05070c,.86f));
   label(d,font,atlas,cut(album->name,40),172,301,35,color(accent));
   label(d,font,atlas,std::to_string(album->tracks.size())+" TRACKS",1747,299,21,muted,Align::right);
   const int start=std::max(0,trackIndex-7);
   for(int i=start;i<(int)album->tracks.size()&&i<start+10;i++){
    const float y=367+(i-start)*51.f;
    if(i==trackIndex)d.rounded_rect({150,y-30,1610,49},7,color(accent,.33f));
    label(d,font,atlas,std::to_string(i+1),174,y,22,color(accent));
    label(d,font,atlas,cut(album->tracks[(size_t)i].name,90),242,y,23,i==trackIndex?white:muted);
   }
   label(d,font,atlas,"UP/DOWN Select     X Play     R1 Play     O Albums",960,1050,21,white,Align::center);
  }else if(screen==2&&album){
   d.rounded_rect({110,238,1700,660},12,color(0x05070d,.88f));
   const auto cover=cached(album->cover);
   image_fill(d,cover,166,297,525,525);
   if(!cover.id)label(d,font,atlas,album->name,437,560,29,color(accent),Align::center);
   auto state=player.snapshot();
   label(d,font,atlas,"NOW PLAYING",778,335,25,color(accent));
   if(!album->tracks.empty())label(d,font,atlas,cut(album->tracks[(size_t)trackIndex].name,52),778,419,36,white);
   label(d,font,atlas,cut(artist?artist->name:"",43)+"  -  "+cut(album->name,35),778,468,23,muted);
   label(d,font,atlas,state.opening?"OPENING...":state.paused?"PAUSED":state.playing?"PLAYING":cut(state.message,55),778,533,21,color(accent));
   d.rounded_rect({778,585,903,15},7,color(0x334050));
   const float progress=state.duration>0?std::clamp((float)(state.seconds/state.duration),0.f,1.f):0.f;
   d.rounded_rect({778,585,std::max(4.f,903*progress),15},7,color(accent));
   label(d,font,atlas,stamp(state.seconds),780,642,24,white);
   label(d,font,atlas,stamp(state.duration),1680,642,24,white,Align::right);
   label(d,font,atlas,"X Pause/Resume     LEFT/RIGHT Seek 10s     L1/R1 Track     TRIANGLE Tracklist     O Back",960,1050,20,white,Align::center);
  }else if(screen==4){
   d.rounded_rect({250,248,1420,574},12,color(0x05070d,.89f));
   label(d,font,atlas,"SETTINGS",960,321,37,color(accent),Align::center);
   const char* options[]={"Rescan music library","Back to albums"};
   for(int i=0;i<2;i++){
    if(i==settingsIndex)d.rounded_rect({450,385+i*86.f,1020,67},10,color(accent,.30f));
    label(d,font,atlas,options[i],960,430+i*86.f,26,white,Align::center);
   }
   label(d,font,atlas,"Local: /data/PSJukebox/Music     USB: /mnt/usb0/PSJukebox/Music",960,715,19,muted,Align::center);
   label(d,font,atlas,"UP/DOWN Choose     X Confirm     O Back",960,1050,21,white,Align::center);
  }else{
   label(d,font,atlas,"NO MUSIC FOUND",960,446,40,color(accent),Align::center);
   label(d,font,atlas,"USB: PSJukebox/Music/Artist/Album  |  requires elfldr :9021",960,523,23,white,Align::center);
   label(d,font,atlas,"SQUARE Rescan    OPTIONS Settings    O Quit",960,1050,20,white,Align::center);
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
  if(exitDialog){
   d.rounded_rect({0,0,1920,1080},0,color(0x000000,.75f));
   d.bordered_rect({550,374,820,330},13,color(0x141720),2,color(accent));
   label(d,font,atlas,"EXIT PSJUKEBOX?",960,469,40,white,Align::center);
   label(d,font,atlas,exitChoice?"[ YES ]                     NO":"YES                     [ NO ]",960,593,29,color(accent),Align::center);
   label(d,font,atlas,"LEFT/RIGHT Select     X Confirm     O Cancel",960,647,19,muted,Align::center);
  }
  renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
  if(++frame%300==0&&screen==2){auto st=player.snapshot();if(st.ended&&album&&trackIndex+1<(int)album->tracks.size())next_track(1);}
 }
}
