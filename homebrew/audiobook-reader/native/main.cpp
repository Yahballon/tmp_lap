// SPDX-License-Identifier: GPL-3.0-or-later
// Audiobook Reader: metal bookshelf UI derived from the same open-source UI library as Ebook Reader.
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "audio_player.hpp"
#include "library.hpp"
#include "chapters.hpp"
#include "chapter_tracking.hpp"
#include "cover_extract.hpp"
#include "audio_cover_store.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "stb_image.h"
#include "usb_bridge.hpp"
#include "audio_log.hpp"
#include <atomic>
#include <mutex>
#include <pthread.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <cstring>
using namespace hui::gfx;
namespace {
constexpr auto steel=0x9cb4ca,white=0xe9f5ff,cyan=0x57d9fb,muted=0x8b9aab;
const Color kSteel=Color::rgb(steel),kWhite=Color::rgb(white),kCyan=Color::rgb(cyan),kMuted=Color::rgb(muted);
std::string cut(std::string s,size_t n=30){if(s.size()>n){s.resize(n-3);s+="...";}return s;}
void txt(DrawList& d,const Font& f,unsigned atlas,const std::string& t,float x,float y,float size,Color c=kWhite,Align a=Align::left){d.text(f,atlas,t,x,y,size,c,a);}
std::string read(const char* path){FILE* f=fopen(path,"rb");if(!f)return {};
 fseek(f,0,SEEK_END);long n=ftell(f);if(n<1||n>20000000){fclose(f);return {};}
 rewind(f);std::string s((size_t)n,0);fread(&s[0],1,s.size(),f);fclose(f);return s;}
std::string stamp(double seconds){int n=(int)std::max(0.,seconds);char buf[40];snprintf(buf,sizeof(buf),"%02d:%02d:%02d",n/3600,(n/60)%60,n%60);return buf;}
void bar(DrawList& d,float x,float y,float w,float h,float progress){
 d.rounded_rect({x,y,w,h},h/2,Color::rgb(0x13202d));
 d.rounded_rect({x,y,std::max(3.f,w*std::clamp(progress,0.f,1.f)),h},h/2,kCyan);
}
void panel(DrawList& d,float x,float y,float w,float h,bool focus=false){
 d.shadow({x,y,w,h},10,13,Color::rgb(0x000000,0.5f));
 d.bordered_rect({x,y,w,h},9,Color::rgb(focus?0x28394b:0x1b2936),focus?3.f:1.f,focus?kCyan:Color::rgb(0x647788));
}
void shelf(DrawList& d,float top){
 // A physically plausible brushed aluminium rail with alternating specular bands.
 d.gradient_rect({70,top,1780,49},0,Color::rgb(0xd4dde8),Color::rgb(0x4d6377));
 d.line(71,top+2,1849,top+2,3,Color::rgb(0xf0f8ff));
 for(int i=0;i<13;i++)d.line(77,top+8+i*3.1f,1841,top+8+i*3.1f,0.5f,
    Color::rgb(i%2?0x91a6ba:0x5d7388,0.25f));
 d.rounded_rect({75,top+43,1770,12},2,Color::rgb(0x0a121c));
}
struct CoverTexture{unsigned id=0;int width=0,height=0;bool checked=false;};
CoverTexture cover_texture(hui::gfx::Renderer& renderer,const std::string& encoded){
 CoverTexture value{};value.checked=true;
 if(encoded.size()<8||encoded.size()>6*1024*1024)return value;
 int w=0,h=0,n=0;
 if(!stbi_info_from_memory((const stbi_uc*)encoded.data(),(int)encoded.size(),&w,&h,&n)||
     w<1||h<1||w>2048||h>2048||(int64_t)w*h>3000000)return value;
 stbi_uc* rgba=stbi_load_from_memory((const stbi_uc*)encoded.data(),(int)encoded.size(),&w,&h,&n,4);
 if(!rgba)return value;
 value.id=renderer.batch().create_texture(w,h,rgba);
 value.width=w;value.height=h;
 stbi_image_free(rgba);return value;
}
void free_cover(CoverTexture& value){
 if(value.id){GLuint id=value.id;glDeleteTextures(1,&id);value.id=0;}
}
void draw_cover(DrawList& d,const CoverTexture& cover,float x,float y,float w,float h){
 if(!cover.id||cover.width<1||cover.height<1)return;
 // Center-crop like Ebook Reader: portrait art fills the physical book.
 const float src=(float)cover.width/(float)cover.height,dst=w/h;
 float u=0,v=0,uw=1,vh=1;
 if(src>dst){uw=dst/src;u=(1-uw)/2;}
 else{vh=src/dst;v=(1-vh)/2;}
 d.image(cover.id,{x,y,w,h},{u,v,uw,vh},Color::rgb(0xffffff),3);
}
void case_art(DrawList& d,const Font& f,unsigned a,const audio::Book& b,float x,float y,bool selected,const CoverTexture& cover){
 constexpr float w=206,h=265;
 unsigned seed=5381;for(unsigned char c:b.title)seed=seed*33+c;
 const unsigned palette[]={0x26435f,0x35576b,0x526472,0x3b4363,0x38576a,0x4d5865,0x314f59};
 if(selected)d.glow({x-4,y-4,w+8,h+8},10,7,kCyan);
 d.shadow({x,y,w,h},6,13,Color::rgb(0x000000,0.65f));
 d.bordered_rect({x,y,w,h},7,Color::rgb(palette[seed%7]),selected?4:2,selected?kCyan:kSteel);
 if(cover.id)draw_cover(d,cover,x+5,y+5,w-10,h-10);
 else {
  d.gradient_rect({x+9,y+8,w-18,86},0,Color::rgb(0x3e5367),Color::rgb(palette[seed%7]));
  d.line(x+29,y+50,x+w-29,y+50,3,kCyan);
  d.line(x+40,y+64,x+w-40,y+64,1,kSteel);
  txt(d,f,a,"M4B AUDIO",x+w/2,y+105,18,kCyan,Align::center);
  txt(d,f,a,cut(b.title,17),x+w/2,y+157,22,kWhite,Align::center);
  txt(d,f,a,cut(b.author,18),x+w/2,y+183,16,kSteel,Align::center);
  for(int j=0;j<9;j++){float mag=(float)((seed>>(j%5))%21);float cx=x+55+j*12.f;d.line(cx,y+213-mag,cx,y+213+mag,3,kCyan);}
 }
 d.rounded_rect({x+4,y+7,8,h-14},1,Color::rgb(0xdbe7f5,0.20f));
 if(b.favorite)d.star(x+w-22,y+20,10,kCyan);
 if(b.position>0&&b.duration>0)bar(d,x+16,y+h-13,w-32,5,(float)(b.position/b.duration));
}
std::vector<std::string> paths(){
 // Never recursively scan the data root: it also stores Logs, Cache and state.
 // The dedicated helper exposes /mnt/usb0/ABooks through elfldr :9021.
 return {audio::root()+"/Books",audio::root(),"/app0/assets/ABooks","/download0/ABooks"};
}
struct OpenTask {
 audio::Book book;
 usbaudio::Entry entry;
 bool remote=false;
};
struct OpenResult {
 std::string source,local,error,title,author;
 double duration=0;
 std::vector<audio::Chapter> chapters;
 std::string coverBytes;
 bool ok=false;
};
std::atomic<bool> g_openBusy{false},g_openCancel{false};
std::atomic<size_t> g_openDone{0},g_openTotal{0};
std::atomic<int> g_openStage{0}; // 1 USB transfer, 2 metadata
std::mutex g_openMutex;
std::vector<OpenResult> g_openResults;
bool transfer_progress(size_t done,size_t total,void*){
 g_openDone.store(done);g_openTotal.store(total);
 return !g_openCancel.load();
}
void* open_worker(void* pointer){
 OpenTask* task=static_cast<OpenTask*>(pointer);
 OpenResult result;
 result.source=task->book.path;
 result.local=task->book.path;
 result.title=task->book.title;
 result.author=task->book.author;
 result.duration=task->book.duration;
 if(task->remote){
  g_openStage.store(1);
  result.ok=usbaudio::fetch(task->entry,result.local,result.error,transfer_progress,nullptr);
 }else result.ok=true;
 if(result.ok&&!g_openCancel.load()){
  g_openStage.store(2);
  result.chapters=audio::chapters(result.local,&result.duration,&result.author,&result.title);
  if(!audio_cover::load(result.source,result.coverBytes)){
   std::string diag;
   if(audio_cover::extract(result.local,result.coverBytes,diag)){
    audio_cover::save(result.source,result.coverBytes);
    audio_log::write("COVER_OK",result.source+" bytes="+std::to_string(result.coverBytes.size()));
   }else{
     std::string why;
     if(usbaudio::is_remote(result.source) &&
        usbaudio::fetch_cover(result.source,result.coverBytes,why)){
       audio_cover::save(result.source,result.coverBytes);
       audio_log::write("COVER_COMPANION_OK",result.source);
     }else audio_log::write("COVER_MISSING",result.source+" embedded="+diag+" companion="+why);
    }
  }
 }else if(g_openCancel.load()){result.ok=false;result.error="Audiobook loading cancelled";}
 if(g_openCancel.load()){result.ok=false;result.error="Audiobook loading cancelled";}
 {std::lock_guard<std::mutex> lock(g_openMutex);
  g_openResults.push_back(std::move(result));
 }
 delete task;
 g_openBusy.store(false);
 return nullptr;
}
struct CoverTask{std::string path,local;};
struct CoverDone{std::string path,encoded;};
std::atomic<bool> g_coverBusy{false};
std::mutex g_coverMutex;
std::vector<CoverDone> g_coverReady;
void* cover_worker(void* arg){
 CoverTask* task=static_cast<CoverTask*>(arg);
 std::string encoded;
 if(!audio_cover::load(task->path,encoded)){
  std::string error;
  if(usbaudio::is_remote(task->path))
   usbaudio::fetch_cover(task->path,encoded,error);
  if(encoded.empty() && !task->local.empty()){
   std::string diag;
   audio_cover::extract(task->local,encoded,diag);
  }
  if(!encoded.empty())audio_cover::save(task->path,encoded);
  else audio_log::write("COVER_LOOKUP",task->path+" "+error);
 }
 {
  std::lock_guard<std::mutex> lock(g_coverMutex);
  g_coverReady.push_back({task->path,std::move(encoded)});
 }
 delete task;
 g_coverBusy.store(false);
 return nullptr;
}
bool start_cover(const audio::Book& book){
 if(g_coverBusy.exchange(true))return false;
 auto* task=new CoverTask{book.path,book.path};
 if(usbaudio::is_remote(book.path)){
  usbaudio::Entry e;
  task->local.clear();
  if(usbaudio::entry_from_remote(book.path,e))
   usbaudio::cached_copy(e,task->local);
 }
 pthread_t thread{};
 if(pthread_create(&thread,nullptr,cover_worker,task)!=0){
  delete task;g_coverBusy.store(false);return false;
 }
 pthread_detach(thread);return true;
}
bool start_open(const audio::Book& book,std::string& error){
 if(g_openBusy.exchange(true))return false;
 g_openCancel.store(false);
 g_openStage.store(0);g_openDone.store(0);g_openTotal.store(0);
 OpenTask* task=new OpenTask;task->book=book;
 if(usbaudio::is_remote(book.path)){
  task->remote=true;
  if(!usbaudio::entry_from_remote(book.path,task->entry)){
   error="Helper catalogue changed; press OPTIONS to refresh";
   delete task;g_openBusy.store(false);return false;
  }
 }
 pthread_t thread{};
 if(pthread_create(&thread,nullptr,open_worker,task)!=0){
  error="Cannot start audiobook loading worker";
  delete task;g_openBusy.store(false);return false;
 }
 pthread_detach(thread);
 return true;
}
}
int main(){
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 hui::gfx::Renderer renderer;if(!renderer.init())hui::sys::park();
 Font font;std::string bytes=read("/app0/assets/fonts/inter-semibold.huifont");
 if(bytes.empty())bytes=read("/data/homebrew/PPSA99784/assets/fonts/inter-semibold.huifont");
 if(!font.load(bytes))hui::sys::park();
 auto atlas=renderer.batch().create_font_texture(font);
 hui::ps5::Pad pad;pad.open();
 const bool logReady=audio_log::prepare();
 audio_log::write("BOOT","Audiobook Reader v0.2.4 started; PPSA99784");
 audio_log::write("DATA","Local: "+audio_log::books_directory()+
                         " Log: "+audio_log::filename());
 std::vector<audio::Book> books;
 usbaudio::Catalog usbCatalog,localCatalog;
 auto refresh=[&](){
  if(g_openBusy.load())return;
  audio_log::write("SCAN_BEGIN","Searching local Books and USB ABooks");
  audio::scan(paths(),books);
  audio_log::write("SCAN_LOCAL_DETAIL",audio::scan_report());
  audio_log::write("SCAN_LOCAL","Books="+std::to_string(books.size())+
     " Path="+audio_log::books_directory());
  usbCatalog=usbaudio::scan();
  localCatalog=usbaudio::scan_local();
  if(localCatalog.viaHelper){
   unsigned directBooks=0;
   const auto prefix=audio::root()+"/Books/";
   for(const auto& b:books)if(b.path.rfind(prefix,0)==0)directBooks++;
   if(!directBooks){
    for(const auto& remote:localCatalog.books){
     audio::Book b;
     b.path=usbaudio::remote_path(remote);
     b.title=audio::clean_title(remote.relative);
     b.author="Local Storage";
     b.bytes=(int64_t)remote.bytes;
     books.push_back(std::move(b));
    }
   }
  }
  // Some jailbreak configurations expose USB mounts directly to the app,
  // without the ELF helper. Try direct read-only discovery as a fallback.
  if(usbCatalog.books.empty()){
   std::vector<audio::Book> direct;
   audio::scan({"/mnt/usb0/ABooks","/mnt/usb1/ABooks"},direct);
   if(!direct.empty()){
    audio_log::write("USB_DIRECT","Found "+std::to_string(direct.size())+
      " M4B books through direct USB mount");
    books.insert(books.end(),direct.begin(),direct.end());
   }else audio_log::write("USB_DIRECT","No directly readable ABooks mount");
  }
  for(const auto& remote:usbCatalog.books){
   audio::Book b;
   b.path=usbaudio::remote_path(remote);
   b.title=audio::clean_title(remote.relative);
   b.author="USB Storage";
   b.bytes=(int64_t)remote.bytes;
   books.push_back(std::move(b));
  }
  audio::load(books);
  std::sort(books.begin(),books.end(),[](const auto& a,const auto& b){return a.title<b.title;});
  audio_log::write("SCAN_DONE","Books="+std::to_string(books.size())+
    " LocalHelper="+std::to_string(localCatalog.books.size())+
    " USB="+std::to_string(usbCatalog.books.size())+
    " Root="+usbCatalog.usbRoot+
    " Status="+usbCatalog.message);
 };
 refresh();
 audio::Player player;
 std::vector<audio::Chapter> chapters;
 std::string message=logReady?
    "Local Books + USB ABooks via elfldr :9021":
    "WARNING: /data/AudioBookReader log directory is not writable";
 std::string loadingTitle,playingPath;
 std::map<std::string,CoverTexture> covers;
 unsigned coverTick=0;
 int selected=0,filter=0,scroll=0,chapterIndex=0,activeChapter=-1;
 bool viewing=false,chapterMode=false,chapterManuallySelected=false,exitDialog=false,closing=false;
 bool list=false;
 unsigned held=0;int holdCooldown=0,saveTicker=0;
 auto visible=[&](){
  std::vector<size_t> result;
  for(size_t i=0;i<books.size();i++)
   if(filter==0||(filter==1&&books[i].position>0)||(filter==2&&books[i].favorite))result.push_back(i);
  return result;
 };
 auto snapshotProgress=[&](){
  if(!viewing||playingPath.empty())return;
  const auto state=player.snapshot();
  for(auto& book:books)if(book.path==playingPath){
   if(state.duration>0)book.duration=state.duration;
   if(state.seconds>0)book.position=state.seconds;
   break;
  }
 };
 for(;;){
  std::array<hui::PadSample,64> events{};auto n=pad.read(events);
  unsigned pressed=0,now=held;
  for(size_t i=0;i<n;i++){pressed|=events[i].buttons&~now;now=events[i].buttons;}
  held=now;
  if(g_openBusy.load()){
   if(pressed&hui::pad_bits::kCircle)g_openCancel.store(true);
   pressed=0;now=0;
  }
  if(--holdCooldown<=0){
   if(now&hui::pad_bits::kUp)pressed|=hui::pad_bits::kUp;
   if(now&hui::pad_bits::kDown)pressed|=hui::pad_bits::kDown;
   if(now&hui::pad_bits::kLeft)pressed|=hui::pad_bits::kLeft;
   if(now&hui::pad_bits::kRight)pressed|=hui::pad_bits::kRight;
   holdCooldown=12;
  }
  // M4B decoding/filesystem access runs on workers. GL uploads occur only here.
  std::vector<CoverDone> finishedCovers;
  {std::lock_guard<std::mutex> lock(g_coverMutex);finishedCovers.swap(g_coverReady);}
  for(const auto& item:finishedCovers){
   auto it=covers.find(item.path);
   // If opening the same USB audiobook already produced an image, do not
   // overwrite it with a stale empty background result.
   if(item.encoded.empty()&&it!=covers.end()&&it->second.id)continue;
   if(it!=covers.end())free_cover(it->second);
   covers[item.path]=cover_texture(renderer,item.encoded);
   audio_log::write(item.encoded.empty()?"COVER_NONE":"COVER_DRAW",
     item.path+" encoded="+std::to_string(item.encoded.size()));
  }
  std::vector<OpenResult> ready;
  {std::lock_guard<std::mutex> lock(g_openMutex);ready.swap(g_openResults);}
  for(auto& result:ready){
   if(!result.ok){
    message="Cannot open audiobook: "+result.error;
    audio_log::write("OPEN_ERROR",result.source+" "+result.error);
    continue;
   }
   auto book=std::find_if(books.begin(),books.end(),[&](const auto& b){return b.path==result.source;});
   if(book==books.end())continue;
   if(!result.title.empty())book->title=result.title;
   if(!result.author.empty())book->author=result.author;
   if(result.duration>0)book->duration=result.duration;
   if(!result.coverBytes.empty()){
    CoverTexture uploaded=cover_texture(renderer,result.coverBytes);
    auto it=covers.find(result.source);
    if(it!=covers.end())free_cover(it->second);
    covers[result.source]=uploaded;
   }
   if(player.start(result.local,book->position)){
    playingPath=book->path;
    chapters=std::move(result.chapters);
    activeChapter=audio::chapter_at(chapters,book->position);
    chapterIndex=std::max(0,activeChapter);
    viewing=true;chapterMode=false;chapterManuallySelected=false;message="";
    audio_log::write("OPEN_OK",playingPath+" local="+result.local);
   }else{
    message="Unable to open cached or local M4B file";
    audio_log::write("PLAYER_ERROR",result.local);
   }
  }
  auto ids=visible();
  selected=std::clamp(selected,0,std::max(0,(int)ids.size()-1));
  if(exitDialog){
   if(pressed&hui::pad_bits::kCircle)exitDialog=false;
   else if(pressed&hui::pad_bits::kCross){exitDialog=false;closing=true;}
  }else if(pressed&hui::pad_bits::kCircle){
   if(chapterMode)chapterMode=false;
   else if(viewing){snapshotProgress();player.stop();viewing=false;activeChapter=-1;playingPath.clear();audio::save(books);}
   else exitDialog=true;
  }else if(viewing){
   auto status=player.snapshot();
   if(pressed&hui::pad_bits::kCross)player.toggle_pause();
   if(pressed&hui::pad_bits::kLeft)player.skip(-15);
   if(pressed&hui::pad_bits::kRight)player.skip(30);
   if(pressed&hui::pad_bits::kL2)player.skip(-60);
   if(pressed&hui::pad_bits::kR2)player.skip(60);
   if(pressed&hui::pad_bits::kTriangle){
    chapterMode=!chapterMode;
    chapterManuallySelected=false;
    if(chapterMode)chapterIndex=std::max(0,activeChapter);
   }
   if(chapterMode){
    if(!chapters.empty()){
     if(pressed&hui::pad_bits::kDown){
      chapterIndex=std::min((int)chapters.size()-1,chapterIndex+1);
      chapterManuallySelected=true;
     }
     if(pressed&hui::pad_bits::kUp){
      chapterIndex=std::max(0,chapterIndex-1);
      chapterManuallySelected=true;
     }
     if(pressed&hui::pad_bits::kSquare){
      player.seek(chapters[chapterIndex].start);
      chapterManuallySelected=false;
     }
    }
   }else{
    if((pressed&hui::pad_bits::kR1)&&!chapters.empty()){
     for(const auto& ch:chapters)if(ch.start>status.seconds+0.2){player.seek(ch.start);break;}
    }
    if((pressed&hui::pad_bits::kL1)&&!chapters.empty()){
     double previous=0;for(const auto& ch:chapters)if(ch.start<status.seconds-2)previous=ch.start;
     player.seek(previous);
    }
   }
   if(status.ended&&!status.playing){snapshotProgress();audio_log::write("PLAY_END",playingPath+" "+status.message);player.stop();viewing=false;activeChapter=-1;playingPath.clear();audio::save(books);message=status.message;}
   if(++saveTicker>=280){saveTicker=0;snapshotProgress();audio::save(books);}
  }else{
   if(pressed&hui::pad_bits::kOptions){refresh();selected=0;message="Library: "+std::to_string(books.size())+" books; "+usbCatalog.message;}
   if(pressed&hui::pad_bits::kR1){filter=(filter+1)%3;selected=0;}
   if(pressed&hui::pad_bits::kL1){filter=(filter+2)%3;selected=0;}
   if(pressed&hui::pad_bits::kR2)list=!list;
   int step=list?1:7;
   if(pressed&hui::pad_bits::kUp)selected=std::max(0,selected-step);
   if(pressed&hui::pad_bits::kDown)selected=std::min((int)ids.size()-1,selected+step);
   if(pressed&hui::pad_bits::kLeft)selected=std::max(0,selected-1);
   if(pressed&hui::pad_bits::kRight)selected=std::min((int)ids.size()-1,selected+1);
   if((pressed&hui::pad_bits::kSquare)&&!ids.empty()){books[ids[selected]].favorite=!books[ids[selected]].favorite;audio::save(books);}
   if((pressed&hui::pad_bits::kCross)&&!ids.empty()){
    const auto& b=books[ids[selected]];
    audio_log::write("OPEN_REQUEST",b.path);
    if(start_open(b,message)){loadingTitle=b.title;message="";}
    else if(message.empty())message="Audiobook loader already running";
   }
  }
  // Keep chapter metadata synchronized with the audio clock EVERY frame.
  // Chapter-list selection stays independent while the user navigates it.
  if(viewing && !chapters.empty()){
   const auto playback=player.snapshot();
   if(playback.playing && !playback.opening && !playback.seeking){
    const int current=audio::chapter_at(chapters,playback.seconds);
    if(current>=0 && current!=activeChapter){
     activeChapter=current;
     audio_log::write("CHAPTER_CHANGE",playingPath+" "+std::to_string(current+1)+
       "/"+std::to_string(chapters.size())+" "+
       chapters[(size_t)current].title);
    }
    if(!chapterManuallySelected && current>=0)chapterIndex=current;
   }
  }
  if(closing && !g_openBusy.load() && !g_coverBusy.load()){
   snapshotProgress();player.stop();audio::save(books);
   audio_log::write("EXIT","Audiobook Reader closed safely");
   for(auto& item:covers)free_cover(item.second);
   pad.close();renderer.release();display.close();hui::sys::quit();
  }
  DrawList d;
  d.gradient_rect({0,0,1920,1080},0,Color::rgb(0x172533),Color::rgb(0x070e18));
  d.gradient_rect({0,0,1920,94},0,Color::rgb(0x536b82),Color::rgb(0x152536));
  d.line(0,92,1920,92,4,kCyan);
  d.bordered_rect({24,19,58,57},10,Color::rgb(0x1c3545),2,kSteel);
  for(int i=0;i<5;i++)d.line(35+i*8,49-(i%3)*7,35+i*8,55+(i%3)*7,3,kCyan);
  txt(d,font,atlas,"AUDIOBOOK READER by YAHBALLON",109,58,31,kWhite);
  txt(d,font,atlas,"v0.2.4",1878,58,22,kCyan,Align::right);
  if(viewing){
   auto st=player.snapshot();
   const auto playing=std::find_if(books.begin(),books.end(),[&](const auto& b){
    return b.path==playingPath;
   });
   const auto& b=playing!=books.end()?*playing:books.front();
   txt(d,font,atlas,"NOW PLAYING",95,163,36,kWhite);
   txt(d,font,atlas,cut(b.title,75),95,222,32,kWhite);
   panel(d,80,255,590,631);
   const auto coverIt=covers.find(b.path);
   case_art(d,font,atlas,b,270,313,false,coverIt==covers.end()?CoverTexture{}:coverIt->second);
   txt(d,font,atlas,cut(b.author,45),375,727,22,kSteel,Align::center);
   panel(d,714,255,1110,631);
   txt(d,font,atlas,st.opening?"LOADING AUDIO...":st.paused?"PAUSED":st.playing?"PLAYING":"STOPPED",770,329,28,kCyan);
   txt(d,font,atlas,stamp(st.seconds)+"  /  "+stamp(st.duration),770,414,42,kWhite);
   bar(d,770,455,978,20,st.duration>0?(float)(st.seconds/st.duration):0.f);
   txt(d,font,atlas,"M4B | Native AAC decoder | Resume enabled",771,521,19,kSteel);
   if(chapterMode){
    txt(d,font,atlas,"CHAPTER LIST",770,580,23,kCyan);
    int begin=std::max(0,chapterIndex-4);
    for(int i=begin;i<(int)chapters.size()&&i<begin+5;i++){
      const float y=615+(i-begin)*48;
      if(i==chapterIndex)d.rounded_rect({763,y-25,985,45},5,Color::rgb(0x31475a));
      if(i==activeChapter){
       d.rounded_rect({764,y-24,5,43},2,kCyan);
       txt(d,font,atlas,"NOW",1670,y+4,17,kCyan,Align::right);
      }
      txt(d,font,atlas,cut(chapters[i].title,48),783,y+4,19,
        i==activeChapter?kCyan:kWhite);
      txt(d,font,atlas,stamp(chapters[i].start),1742,y+4,17,kSteel,Align::right);
    }
   }else{
    const int liveChapter=activeChapter>=0?activeChapter:audio::chapter_at(chapters,st.seconds);
    txt(d,font,atlas,chapters.empty()?"NO EMBEDDED CHAPTERS":
      "CHAPTER "+std::to_string(liveChapter+1)+" / "+std::to_string(chapters.size()),
      770,605,23,kCyan);
    txt(d,font,atlas,chapters.empty()?"No embedded chapters":
      cut(chapters[(size_t)std::max(0,liveChapter)].title,57),770,652,24,kWhite);
    txt(d,font,atlas,cut(st.message,70),770,748,20,kMuted);
   }
   txt(d,font,atlas,"X Play/Pause  LEFT -15s  RIGHT +30s  L2/R2 +/-60s  L1/R1 Chapter  TRIANGLE List  O Library",960,1015,20,kWhite,Align::center);
   if(chapterMode)txt(d,font,atlas,"UP/DOWN Choose chapter  SQUARE Jump to chapter",960,1050,17,kCyan,Align::center);
  }else{
   txt(d,font,atlas,"MY AUDIOBOOK LIBRARY",84,162,37,kWhite);
   txt(d,font,atlas,std::to_string(books.size())+" audiobooks",590,160,21,kMuted);
   const char* filters[]={"All","In Progress","Favorites"};
   for(int i=0;i<3;i++){float x=1195+i*218.f;panel(d,x,124,200,66,i==filter);txt(d,font,atlas,filters[i],x+100,166,20,i==filter?kCyan:kWhite,Align::center);}
   d.rounded_rect({63,228,1797,750},6,Color::rgb(0x0b1622));
   for(int row=0;row<2;row++)shelf(d,592+row*356);
   const int count=(int)ids.size(),perPage=list?9:14;
   scroll=count?selected/perPage:0;
   if(!closing&&!g_coverBusy.load()&&!g_openBusy.load()&&++coverTick%20==0){
    for(int i=scroll*perPage;i<count&&i<(scroll+1)*perPage;i++){
     const auto& b=books[ids[i]];
     if(covers.find(b.path)!=covers.end())continue;
     if(start_cover(b))break;
    }
   }
   if(covers.size()>54){
    auto it=covers.begin();
    while(covers.size()>40&&it!=covers.end()){
     free_cover(it->second);
     it=covers.erase(it);
    }
   }
   for(int i=scroll*perPage;i<count&&i<(scroll+1)*perPage;i++){
    const audio::Book& b=books[ids[i]];
    const int cell=i-scroll*perPage;
    if(list){
     const float y=254+cell*77.f;
     panel(d,88,y,1747,68,i==selected);
     txt(d,font,atlas,cut(b.title,60),112,y+43,25,kWhite);
     txt(d,font,atlas,stamp(b.position)+" / "+stamp(b.duration),1800,y+42,20,kCyan,Align::right);
    }else{
     const int row=cell/7,col=cell%7;
     const float x=117+col*248.f,y=284+row*354.f;
     const auto it=covers.find(b.path);
     case_art(d,font,atlas,b,x,y,i==selected,it==covers.end()?CoverTexture{}:it->second);
    }
   }
   if(count==0){txt(d,font,atlas,"No M4B audiobooks found.",960,455,30,kWhite,Align::center);
    txt(d,font,atlas,"USB: /mnt/usb0/ABooks     Local: /data/AudioBookReader/Books",960,510,21,kSteel,Align::center);
    txt(d,font,atlas,cut(usbCatalog.message,115),960,554,18,kMuted,Align::center);}
   txt(d,font,atlas,"PAGE "+std::to_string(scroll+1)+" / "+std::to_string(std::max(1,(count+perPage-1)/perPage)),87,1009,18,kSteel);
   txt(d,font,atlas,"X Play  L1/R1 Filter  SQUARE Favourite  OPTIONS Refresh  R2 View  O Exit",960,1052,20,kWhite,Align::center);
  }
  if(!message.empty())txt(d,font,atlas,cut(message,85),960,1000,17,kCyan,Align::center);
  if(g_openBusy.load()){
   const size_t total=g_openTotal.load(),done=g_openDone.load();
   const int stage=g_openStage.load();
   const float fraction=total?std::min(1.f,(float)((double)done/total)):0.f;
   d.rounded_rect({0,100,1920,980},0,Color::rgb(0x040914,0.83f));
   panel(d,278,345,1364,350,true);
   txt(d,font,atlas,stage==1?"COPYING AUDIOBOOK FROM USB":"OPENING AUDIOBOOK",960,409,36,kWhite,Align::center);
   txt(d,font,atlas,cut(loadingTitle,65),960,461,23,kSteel,Align::center);
   bar(d,400,535,1120,30,stage==1?fraction:1.f);
   if(stage==1)
    txt(d,font,atlas,std::to_string((int)(fraction*100))+"%   "+
       std::to_string(done/1048576)+" / "+std::to_string((total+1048575)/1048576)+
       " MiB",960,612,24,kCyan,Align::center);
   else txt(d,font,atlas,"Loading M4B metadata and chapters...",960,609,23,kCyan,Align::center);
   txt(d,font,atlas,g_openCancel.load()?"Cancelling...":"CIRCLE Cancel",960,659,20,kMuted,Align::center);
  }
  if(exitDialog){
   d.rounded_rect({0,100,1920,980},0,Color::rgb(0x030810,0.81f));
   panel(d,528,347,865,359,true);
   txt(d,font,atlas,"EXIT AUDIOBOOK READER?",960,439,36,kWhite,Align::center);
   txt(d,font,atlas,"Your listening progress will be saved.",960,507,24,kSteel,Align::center);
   txt(d,font,atlas,"X YES                             O NO",960,601,28,kCyan,Align::center);
  }
  renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
