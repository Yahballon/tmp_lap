// SPDX-License-Identifier: GPL-3.0-or-later
#include "inventory.hpp"
#include "helper_bridge.hpp"
#include "action_bridge.hpp"
#include "shadowmount_api.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_FAILURE_STRINGS
#include "stb_image.h"
#include <map>
#include <mutex>
#include <atomic>
#include <pthread.h>
#include "logger.hpp"
#include "crash_report.hpp"
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include <array>
#include <cstdio>
#include <sys/stat.h>
#include <ctime>
#include <string>
using namespace hui::gfx;
namespace {
Color color(unsigned v,float a=1){return Color::rgb(v,a);}
void txt(DrawList& d,const Font& f,unsigned t,const std::string& s,float x,float y,int sz=21,Color c=color(0xf1f8ff),Align a=Align::left){d.text(f,t,s,x,y,sz,c,a);}
void panel(DrawList& d,float x,float y,float w,float h,bool active=false){d.bordered_rect({x,y,w,h},15,color(active?0x072856:0x061524),active?3:1.5,color(active?0x36c6ff:0x24547c));}
std::string read(const char* p){FILE* f=fopen(p,"rb");if(!f)return {};std::string s;char b[4096];size_t n;while((n=fread(b,1,sizeof b,f))>0&&s.size()<500000)s.append(b,n);fclose(f);return s;}
void trash(DrawList& d,float x,float y,float s){d.rounded_rect({x-5*s,y,65*s,12*s},4*s,color(0xff294d));d.bordered_rect({x,y+14*s,54*s,60*s},8*s,color(0x5b091e),3*s,color(0xff3d63));for(int i=0;i<3;i++)d.rounded_rect({x+(13+13*i)*s,y+25*s,5*s,36*s},2*s,color(0xff6d85));}
bool visible(const uninstaller::Item& a,int tab){return tab==0||(tab==1&&a.kind==uninstaller::Kind::Installed)||(tab==2&&a.kind==uninstaller::Kind::Homebrew)||(tab==3&&a.kind==uninstaller::Kind::UsbPackage);}
int count(const std::vector<uninstaller::Item>& v,int tab){int n=0;for(const auto& a:v)if(visible(a,tab))++n;return n;}
const uninstaller::Item* nth(const std::vector<uninstaller::Item>& v,int tab,int n){for(const auto& a:v)if(visible(a,tab)&&n--==0)return &a;return nullptr;}
void log(const std::string& s){uninstaller::diag::write("EVENT",s.c_str());}
int scanRootsReadable=0,scanRootsMissing=0,scanRootsDenied=0;
void scanLog(const char* phase,const char* path){
  uninstaller::diag::write(phase,path);
  const std::string s=path?path:"";
  if(std::strcmp(phase,"SCAN_ROOT_DONE")==0||std::strcmp(phase,"PKG_ROOT_DONE")==0)
    scanRootsReadable++;
  else if(std::strcmp(phase,"SCAN_ROOT_ERROR")==0||std::strcmp(phase,"PKG_ROOT_ERROR")==0){
    if(s.find("errno=13")!=std::string::npos)scanRootsDenied++;
    else scanRootsMissing++;
  }
}

// Icons are transferred on demand. Network traffic runs on a worker thread;
// OpenGL texture uploads happen ONLY on the renderer thread.
struct IconJob {int id=-1;std::string path,bytes;unsigned epoch=0;bool ok=false;};
std::mutex iconMutex;
std::atomic<bool> iconBusy{false};
bool iconReady=false;
IconJob iconResult;
void* icon_worker(void* ptr){
  IconJob* job=(IconJob*)ptr;
  job->ok=uninstaller::bridge::fetch_icon(job->id,job->bytes);
  {std::lock_guard<std::mutex> lock(iconMutex);iconResult=std::move(*job);iconReady=true;}
  delete job;iconBusy=false;return nullptr;
}
bool queue_icon(const uninstaller::Item& item,unsigned epoch){
  if(item.remoteIndex<0||iconBusy.exchange(true))return false;
  IconJob* job=new IconJob;job->id=item.remoteIndex;job->path=item.path;job->epoch=epoch;
  pthread_t thread{};
  if(pthread_create(&thread,nullptr,icon_worker,job)!=0){
    delete job;iconBusy=false;return false;
  }
  pthread_detach(thread);return true;
}

struct ScanFinish {
 std::vector<uninstaller::Item> items;
 std::vector<uninstaller::bridge::Volume> volumes;
 std::string message;
};
std::mutex scanMutex;
std::atomic<bool> scanBusy{false};
ScanFinish* scanReady=nullptr;
std::string scanStep="Starting inventory";
void updateScanStep(const std::string& step){
 std::lock_guard<std::mutex> guard(scanMutex);
 scanStep=step;
}
void* scanWorker(void*){
 ScanFinish* result=new ScanFinish;
 updateScanStep("Reading local homebrew folders");
 result->items=uninstaller::scan(scanLog);
 updateScanStep("Scanning games, homebrew and USB via elfldr");
 const auto helper=uninstaller::bridge::scan("/app0/unscan.elf",scanLog);
 uninstaller::bridge::merge(result->items,helper);
 result->volumes=helper.volumes;
 result->message=helper.message;
 {std::lock_guard<std::mutex> guard(scanMutex);scanReady=result;}
 scanBusy.store(false);
 return nullptr;
}
bool startScan(){
 if(scanBusy.exchange(true))return false;
 updateScanStep("Starting inventory");
 pthread_t thread{};
 if(pthread_create(&thread,nullptr,scanWorker,nullptr)!=0){
  scanBusy.store(false);
  updateScanStep("Could not launch scan thread");
  return false;
 }
 pthread_detach(thread);
 return true;
}

struct RemovalStatus {
 std::mutex mutex;
 bool active=false,done=false,success=false;
 int percent=-1;
 unsigned long long checkMillis=0;
 std::string message;
};
RemovalStatus removal;
void removalLog(const char* phase,const char* details){
 scanLog(phase,details);
 if(!phase||!details)return;
 if(std::strcmp(phase,"REMOVE_PROGRESS")==0){
   const auto p=uninstaller::actions::parseProgress(details);
   if(!p.valid)return;
   std::lock_guard<std::mutex> guard(removal.mutex);
   if(p.deleting){
     const unsigned long long span=p.elapsedMs>removal.checkMillis?
           p.elapsedMs-removal.checkMillis:1;
     removal.percent=p.total?(int)(p.current*100ull/p.total):-1;
     removal.message="Turbo Delete: "+std::to_string(p.current)+"/"+
       std::to_string(p.total)+" entries | "+std::to_string(p.current*1000ull/span)+
       "/s | "+std::to_string(span/1000)+"s";
   }else{
     if(p.total>0)removal.checkMillis=p.elapsedMs;
     removal.message="Safety check: "+std::to_string(p.current)+
         " entries verified ("+std::to_string(p.elapsedMs/1000)+"s)";
     removal.percent=-1;
   }
 }else if(std::strcmp(phase,"REMOVE_STAGE")==0){
   std::lock_guard<std::mutex> guard(removal.mutex);
   if(std::strcmp(details,"source_quarantine")==0)
      removal.message="Game hidden from ShadowMount scan. Preparing cleanup...";
   else if(std::strcmp(details,"source_delete")==0)
      removal.message="Turbo Delete: checking game files...";
 }
}
struct RemovalRequest {
 uninstaller::Item item;
 std::string source;
};
void* removeWorker(void* arg){
 RemovalRequest* job=(RemovalRequest*)arg;
 uninstaller::diag::write("UNIFIED_UNINSTALL",job->item.id.c_str());
 const auto mode=job->item.kind==uninstaller::Kind::Installed?
      uninstaller::actions::Mode::UninstallAll:uninstaller::actions::Mode::Normal;
 const auto result=uninstaller::actions::execute(
     job->item,removalLog,"/app0/unremove.elf",mode,job->source);
 {std::lock_guard<std::mutex> guard(removal.mutex);
  removal.active=false;removal.done=true;
  removal.success=result.accepted;
  removal.message=result.detail;
 }
 delete job;
 return nullptr;
}
bool runRemoval(const uninstaller::Item& item,const std::string& source){
 {std::lock_guard<std::mutex> guard(removal.mutex);
  if(removal.active)return false;
  removal.active=true;removal.done=false;removal.success=false;
  removal.percent=-1;removal.checkMillis=0;
  removal.message="Uninstalling title and matching USB source...";
 }
 pthread_t thread{};
 RemovalRequest* request=new RemovalRequest{item,source};
 if(pthread_create(&thread,nullptr,removeWorker,request)!=0){
  delete request;
  std::lock_guard<std::mutex> guard(removal.mutex);
  removal.active=false;removal.message="Removal worker could not start";
  return false;
 }
 pthread_detach(thread);
 return true;
}
std::uint32_t decode_icon(hui::gfx::Renderer& renderer,const std::string& png){
  if(png.empty()||png.size()>3*1024*1024)return 0;
  int w=0,h=0,ch=0;
  stbi_uc* pixels=stbi_load_from_memory((const stbi_uc*)png.data(),(int)png.size(),&w,&h,&ch,4);
  if(!pixels)return 0;
  std::uint32_t texture=0;
  if(w>=32&&h>=32&&w<=1024&&h<=1024)
     texture=renderer.batch().create_texture(w,h,pixels);
  stbi_image_free(pixels);return texture;
}

void render(DrawList& d,const Font& f,unsigned a,const std::vector<uninstaller::Item>& items,int tab,int selected,int offset,bool confirm,const std::string& msg,
 const std::map<std::string,std::uint32_t>& icons,
 bool scanning,unsigned frame,const std::string& phase,
 bool shadowConfirm,const std::string& shadowSource,bool shadowBusy,int shadowPercent,const std::string& shadowMessage){
 const Color white=color(0xf1f8ff),muted=color(0x9eb7d7);
 d.gradient_rect({0,0,1920,1080},0,color(0x061a31),color(0x020512));
 d.gradient_rect({1250,0,670,1050},0,color(0x051b3c,.55f),color(0x210515,.55f));
 trash(d,70,34,.77f);txt(d,f,a,"Uninstaller",168,85,55);
 txt(d,f,a,"Remove games and homebrew apps from your console.",168,123,21,muted);
 txt(d,f,a,"Clean Storage.  More Space.  A Smoother Console.",1190,83,19,muted);
 time_t now=time(nullptr);tm* t=localtime(&now);char b[20]="--:--";if(t)strftime(b,sizeof b,"%H:%M",t);
 txt(d,f,a,b,1880,82,29,white,Align::right);
 panel(d,30,145,682,863);panel(d,723,145,1167,863);
 const char* cats[]={"All","Games","Homebrew","USB PKGs"};
 for(int i=0;i<4;i++){float x=43+i*165;panel(d,x,160,160,52,i==tab);txt(d,f,a,std::string(cats[i])+" ("+std::to_string(count(items,i))+")",x+80,194,17,white,Align::center);}
 for(int i=offset;i<count(items,tab)&&i<offset+11;i++){
   auto* it=nth(items,tab,i);if(!it)continue;float y=233+(i-offset)*68;
   panel(d,44,y,653,65,i==selected);
   d.rounded_rect({58,y+10,47,44},8,color(it->kind==uninstaller::Kind::UsbPackage?0x543046:0x094482));
   const auto tex=icons.find(it->path);
   if(tex!=icons.end()&&tex->second)
     d.image(tex->second,{58,y+10,47,44},kFullUv,white,8);
   else
     txt(d,f,a,it->kind==uninstaller::Kind::UsbPackage?"P":it->kind==uninstaller::Kind::Installed?"G":"H",81,y+40,21,white,Align::center);
   std::string name=it->name.size()>37?it->name.substr(0,34)+"...":it->name;
   txt(d,f,a,name,124,y+29,20);
   txt(d,f,a,it->remote?(uninstaller::actions::eligible(*it)?"Managed via elfldr":"Protected title"):it->kind==uninstaller::Kind::UsbPackage?"USB PKG file":it->kind==uninstaller::Kind::Homebrew?"Homebrew app":"Installed game / app",124,y+53,16,muted);
   txt(d,f,a,it->sizeKnown?uninstaller::bytes(it->size):"--",678,y+38,17,muted,Align::right);
 }
 if(count(items,tab)==0)txt(d,f,a,"No accessible titles found",100,300,22,muted);
 const auto* item=nth(items,tab,selected);
 // Single wide detail panel: console storage telemetry panel is intentionally removed.
 panel(d,1431,196,422,411);
 if(item&&icons.count(item->path)&&icons.at(item->path))
   d.image(icons.at(item->path),{1491,253,302,300},kFullUv,white,14);
 else trash(d,1585,301,2.0f);
 txt(d,f,a,"SELECTED TITLE",760,218,18,color(0x48bafa));
 if(item){
  std::string title=item->name.size()>35?item->name.substr(0,32)+"...":item->name;
  txt(d,f,a,title,760,272,31,white);
  txt(d,f,a,item->kind==uninstaller::Kind::UsbPackage?"External PKG archive":
    item->kind==uninstaller::Kind::Homebrew?"Homebrew folder":
    "Game / installed title",760,312,20,muted);
  d.line(760,355,1397,355,1,color(0x36587b));
  txt(d,f,a,"Title ID",765,411,21,muted);
  txt(d,f,a,item->id.empty()?"Not available":item->id,1381,411,21,white,Align::right);
  txt(d,f,a,"Version",765,468,21,muted);
  txt(d,f,a,item->version.empty()?"Unknown":item->version,1381,468,21,white,Align::right);
  txt(d,f,a,"Install Size",765,525,21,muted);
  txt(d,f,a,item->sizeKnown?uninstaller::bytes(item->size):"Not measured",1381,525,21,white,Align::right);
  txt(d,f,a,"Location",765,582,21,muted);
  std::string path=item->root;
  if(path.size()>35)path=path.substr(0,32)+"...";
  txt(d,f,a,path,1381,582,19,white,Align::right);
  d.line(760,655,1840,655,1,color(0x36587b));
  d.bordered_rect({760,715,1081,102},14,color(0x880c25),3,color(0xff4366));
  txt(d,f,a,uninstaller::actions::eligible(*item)?
     (item->kind==uninstaller::Kind::UsbPackage?"Delete USB PKG":
      item->kind==uninstaller::Kind::Homebrew?"Delete Homebrew":"Uninstall")
     :"Read-only title",1300,781,32,white,Align::center);
  if(item->kind==uninstaller::Kind::Installed)
    txt(d,f,a,"X / Square: Uninstall game and associated USB source",770,868,20,color(0xff8494));
  txt(d,f,a,"Save data stays untouched",770,894,21,muted);
  txt(d,f,a,"Installed titles are removed through the PS5 installer service",770,938,18,muted);
 }else{
  txt(d,f,a,"Select a game, application or USB package",770,692,27,muted);
 }
 d.bordered_rect({0,1027,1920,53},0,color(0x020d1a),1,color(0x2b496d));
 txt(d,f,a,"D-pad Select    L1/R1 Categories    X / Square: Uninstall    Triangle: Refresh    Circle: Back",45,1063,17,muted);
 txt(d,f,a,"by YAHBALLON   v0.1.12",1887,1063,18,muted,Align::right);
 // Status messages are intentionally unobtrusive: never cover the game list.
 if(!msg.empty()&&msg!="ok"){
   const std::string brief=msg.size()>125?msg.substr(0,122)+"...":msg;
   txt(d,f,a,brief,745,991,16,color(0x99bad9));
 }
 if(scanning){
  panel(d,1055,828,560,142);
  txt(d,f,a,"Scanning games, apps and USB...",1077,866,21,white);
  txt(d,f,a,phase,1077,903,16,muted);
  d.rounded_rect({1077,927,511,17},7,color(0x1a3958));
  const float offset=(float)(frame%135)/135.0f;
  d.rounded_rect({1080+offset*377,930,125,11},5,color(0x2aaaff));
 }
 if(shadowBusy){
  txt(d,f,a,shadowMessage,755,971,18,color(0xffb276));
  d.rounded_rect({1430,984,398,12},5,color(0x263d58));
  if(shadowPercent>=0)
    d.rounded_rect({1432,986,394.0f*shadowPercent/100.0f,8},4,color(0xff704d));
  else
    d.rounded_rect({1432+(frame%110)*2.5f,986,114,8},4,color(0xff704d));
 }
 if(confirm){
  d.bordered_rect({367,345,1190,345},20,color(0x05162b),3,color(0xff466a));
  txt(d,f,a,shadowConfirm?"CONFIRM COMPLETE GAME REMOVAL":"CONFIRM REMOVAL",
      962,398,36,color(0xff607a),Align::center);
  txt(d,f,a,item?item->name:"Unknown",962,455,26,white,Align::center);
  if(shadowConfirm){
    const std::string source=shadowSource.size()>104?shadowSource.substr(0,101)+"...":shadowSource;
    txt(d,f,a,source,962,511,19,white,Align::center);
    txt(d,f,a,"X: Remove game + source    Circle: Cancel",962,579,23,white,Align::center);
    txt(d,f,a,"Permanently deletes the matching USB source. Saves not targeted.",962,632,19,color(0xff8494),Align::center);
  }else{
    txt(d,f,a,"X: Confirm selected removal     Circle: Cancel",962,567,25,white,Align::center);
    txt(d,f,a,item&&item->kind==uninstaller::Kind::Installed?
      "Scans for a matching ShadowMount USB folder. Saves are not targeted.":
      "This permanently deletes the selected USB files.",962,622,20,muted,Align::center);
  }
 }
}
}
int main(){
 uninstaller::diag::write("BOOT","main entered");
 const bool crashReady=devbridge_crash::install(uninstaller::diag::kCrash);
 uninstaller::diag::write("CRASH_REPORTER",crashReady?"installed":"unavailable");
 uninstaller::diag::write("DISPLAY","open begin");
 hui::ps5::Display display;
 if(!display.open()){uninstaller::diag::write("DISPLAY","open failed");hui::sys::park();}
 uninstaller::diag::write("DISPLAY","open ok");
 uninstaller::diag::write("RENDERER","init begin");
 Renderer renderer;
 if(!renderer.init()){uninstaller::diag::write("RENDERER","init failed");hui::sys::park();}
 uninstaller::diag::write("RENDERER","init ok");
 uninstaller::diag::write("FONT","read begin");
 Font font;std::string fonts=read("/app0/assets/fonts/inter-semibold.huifont");
 if(fonts.empty())fonts=read("/data/homebrew/PPSA99986/assets/fonts/inter-semibold.huifont");
 uninstaller::diag::write("FONT",fonts.empty()?"missing":"bytes loaded");
 if(!font.load(fonts)){uninstaller::diag::write("FONT","font.load failed");hui::sys::park();}
 uninstaller::diag::write("FONT","font.load ok");
 uninstaller::diag::write("FONT","atlas create begin");
 unsigned atlas=renderer.batch().create_font_texture(font);
 uninstaller::diag::write("FONT","atlas create ok");
 uninstaller::diag::write("PAD","open begin");
 hui::ps5::Pad pad;pad.open();
 uninstaller::diag::write("PAD","open returned");
 std::vector<uninstaller::Item> items;
 std::map<std::string,std::uint32_t> iconTextures;
 unsigned iconEpoch=0;
 // PS5 statvfs is not proven safe with this runtime; avoid the possible
 // unimplemented libc entry until supported console telemetry is confirmed.
 uninstaller::diag::write("STORAGE","statvfs skipped: PS5 support unverified");
 int tab=0,selected=0,offset=0;bool confirm=false,shadowConfirm=false;
 std::string shadowSource;bool shadowDoneHandled=false;
 unsigned frames=0;
 bool firstFrame=true;
 unsigned prev=0;std::string notice;
 uninstaller::diag::write("BOOT","initialization finished; first frame pending");
 for(;;){
  bool sourceBusy=false,sourceDone=false,sourceSuccess=false;
  int sourcePercent=-1;std::string sourceMessage;
  {std::lock_guard<std::mutex> lock(removal.mutex);
    sourceBusy=removal.active;sourceDone=removal.done;
    sourceSuccess=removal.success;sourceMessage=removal.message;
    sourcePercent=removal.percent;
  }
  if(sourceDone&&!shadowDoneHandled){
    shadowDoneHandled=true;
    notice=sourceMessage;
    uninstaller::diag::write("UNIFIED_RESULT",sourceMessage.c_str());
    if(sourceSuccess&&!scanBusy.load())startScan();
  }
  if(sourceBusy)shadowDoneHandled=false;
  ScanFinish* fresh=nullptr;
  {std::lock_guard<std::mutex> guard(scanMutex);
    fresh=scanReady;scanReady=nullptr;
  }
  if(fresh){
    items=std::move(fresh->items);
    selected=offset=0;
    notice=items.empty()?fresh->message:"";
    log("Async scan found "+std::to_string(items.size())+" titles");
    delete fresh;
  }
  std::array<hui::PadSample,64> s{};unsigned pressed=0,buttons=prev;
  size_t n=pad.read(s);bool connected=false;
  for(size_t i=0;i<n;i++)if(s[i].connected){connected=true;buttons=s[i].buttons;
    if(s[i].left_y<60)buttons|=hui::pad_bits::kUp;
    if(s[i].left_y>195)buttons|=hui::pad_bits::kDown;
    pressed|=buttons&~prev;prev=buttons;
  }
  if(!connected)prev=0;
  if(scanBusy.load()||sourceBusy)pressed=0;
  if(pressed&hui::pad_bits::kCircle){if(confirm){confirm=false;shadowConfirm=false;}else{uninstaller::bridge::stop_icons();for(const auto& entry:iconTextures)if(entry.second){GLuint texture=entry.second;glDeleteTextures(1,&texture);}pad.close();renderer.release();display.close();hui::sys::quit();}}
  if(!confirm){
    if(pressed&hui::pad_bits::kL1){tab=(tab+3)%4;selected=offset=0;}
    if(pressed&hui::pad_bits::kR1){tab=(tab+1)%4;selected=offset=0;}
    if(pressed&hui::pad_bits::kLeft){tab=(tab+3)%4;selected=offset=0;}
    if(pressed&hui::pad_bits::kRight){tab=(tab+1)%4;selected=offset=0;}
    if(pressed&hui::pad_bits::kUp){if(selected>0)--selected;if(selected<offset)offset=selected;}
    if(pressed&hui::pad_bits::kDown){if(selected+1<count(items,tab))++selected;if(selected>=offset+11)offset=selected-10;}
    if(pressed&hui::pad_bits::kTriangle){
      uninstaller::diag::write("REFRESH","starting");
      uninstaller::bridge::stop_icons();
      for(const auto& t:iconTextures)if(t.second){GLuint tex=t.second;glDeleteTextures(1,&tex);}
      iconTextures.clear();++iconEpoch;
      notice.clear();
      if(!startScan())notice="Cannot start scan";
    }
    if(pressed&(hui::pad_bits::kCross|hui::pad_bits::kSquare)){
      const auto* x=nth(items,tab,selected);
      if(x&&uninstaller::actions::eligible(*x)){
        if(x->kind==uninstaller::Kind::Installed&&x->remote){
          uninstaller::bridge::stop_icons();
          const auto probe=uninstaller::actions::execute(
              *x,scanLog,"/app0/unremove.elf",uninstaller::actions::Mode::ProbeGame);
          if(probe.code!=0){
            notice="USB source scan failed: "+probe.detail;
          }else{
            shadowSource=probe.source;
            shadowConfirm=!shadowSource.empty();
            confirm=true;
            uninstaller::diag::write("UNIFIED_PREFLIGHT",
               shadowConfirm?shadowSource.c_str():"No matching USB source");
          }
        }else{
          shadowSource.clear();shadowConfirm=false;confirm=true;
        }
      }else if(x)notice="This source cannot be safely removed";
    }
  }else if(pressed&hui::pad_bits::kCross){
    const auto* x=nth(items,tab,selected);
    if(x&&uninstaller::actions::eligible(*x)){
      uninstaller::diag::write("REMOVE_ATTEMPT",x->path.c_str());
      if(x->remote){
        uninstaller::bridge::stop_icons();
        if(runRemoval(*x,shadowSource)){
          notice="Removing selected title and any verified USB source...";
          shadowDoneHandled=false;
        }else notice="Cannot launch removal worker";
      }else{
        const int rc=uninstaller::removeFiles(*x);
        notice=rc==0?"Selected files removed":"Removal rejected: "+std::to_string(rc);
        if(rc==0&&!startScan())notice="Removed, but refresh could not start";
      }
    }
    shadowConfirm=false;shadowSource.clear();confirm=false;
  }

  // Complete at most one image decode per frame.
  {
    IconJob finished;bool ready=false;
    {std::lock_guard<std::mutex> lock(iconMutex);
      if(iconReady){finished=std::move(iconResult);iconReady=false;ready=true;}
    }
    if(ready&&finished.epoch==iconEpoch)
      iconTextures[finished.path]=finished.ok?decode_icon(renderer,finished.bytes):0;
  }
  if(!iconBusy.load()){
    const auto* current=nth(items,tab,selected);
    if(current&&current->remoteIndex>=0&&!iconTextures.count(current->path))
      queue_icon(*current,iconEpoch);
    else{
      for(int row=offset;row<count(items,tab)&&row<offset+11;row++){
        const auto* candidate=nth(items,tab,row);
        if(candidate&&candidate->remoteIndex>=0&&!iconTextures.count(candidate->path)){
          queue_icon(*candidate,iconEpoch);break;
        }
      }
    }
  }
  if(firstFrame)uninstaller::diag::write("FIRST_FRAME","draw begin");
  std::string phase;
  {std::lock_guard<std::mutex> guard(scanMutex);phase=scanStep;}
  DrawList ui;render(ui,font,atlas,items,tab,selected,offset,confirm,notice,iconTextures,scanBusy.load(),frames++,phase,
     shadowConfirm,shadowSource,sourceBusy,sourcePercent,sourceMessage);
  if(firstFrame)uninstaller::diag::write("FIRST_FRAME","draw done");
  renderer.begin();renderer.draw(ui);renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();
  if(firstFrame){
    firstFrame=false;
    uninstaller::diag::write("FIRST_FRAME","present ok");
    uninstaller::diag::write("SCAN","starting async inventory");
    if(!startScan())notice="Cannot start initial scan";
  }
  hui::sys::sleep_us(16000);
 }
}
