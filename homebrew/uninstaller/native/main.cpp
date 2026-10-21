// SPDX-License-Identifier: GPL-3.0-or-later
#include "inventory.hpp"
#include "helper_bridge.hpp"
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

void render(DrawList& d,const Font& f,unsigned a,const std::vector<uninstaller::Item>& items,int tab,int selected,int offset,bool confirm,const std::string& msg,const uninstaller::Storage& disk,
 const std::map<std::string,std::uint32_t>& icons,const std::string& driveName,int activeDrive,int driveCount){
 const Color white=color(0xf1f8ff),muted=color(0x9eb7d7);
 d.gradient_rect({0,0,1920,1080},0,color(0x061a31),color(0x020512));
 d.gradient_rect({1250,0,670,1050},0,color(0x051b3c,.55f),color(0x210515,.55f));
 trash(d,70,34,.77f);txt(d,f,a,"Uninstaller",168,85,55);
 txt(d,f,a,"Remove games and homebrew apps from your console.",168,123,21,muted);
 txt(d,f,a,"Clean Storage.  More Space.  A Smoother Console.",1190,83,19,muted);
 time_t now=time(nullptr);tm* t=localtime(&now);char b[20]="--:--";if(t)strftime(b,sizeof b,"%H:%M",t);
 txt(d,f,a,b,1880,82,29,white,Align::right);
 panel(d,30,145,682,863);panel(d,723,145,615,863);panel(d,1349,145,541,863);
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
   txt(d,f,a,it->remote?"Read-only via elfldr":it->kind==uninstaller::Kind::UsbPackage?"USB PKG file":it->kind==uninstaller::Kind::Homebrew?"Homebrew app":"Installed game / app",124,y+53,16,muted);
   txt(d,f,a,it->sizeKnown?uninstaller::bytes(it->size):"--",678,y+38,17,muted,Align::right);
 }
 if(count(items,tab)==0)txt(d,f,a,"No accessible titles found",100,300,22,muted);
 const auto* item=nth(items,tab,selected);
 panel(d,740,163,579,273);
 if(item && icons.count(item->path) && icons.at(item->path))
   d.image(icons.at(item->path),{904,169,255,260},kFullUv,white,14);
 else trash(d,988,237,1.75f);
 if(item){
  txt(d,f,a,item->name.size()>34?item->name.substr(0,31)+"...":item->name,755,480,27);
  txt(d,f,a,item->kind==uninstaller::Kind::UsbPackage?"External PKG archive":item->kind==uninstaller::Kind::Homebrew?"Homebrew folder":"Game / installed title (read-only)",755,518,19,muted);
  d.line(758,552,1298,552,1,color(0x36587b));
  txt(d,f,a,"Title ID",761,591,20,muted);txt(d,f,a,item->id.empty()?"Not available":item->id,1290,591,20,white,Align::right);
  txt(d,f,a,"Version",761,630,20,muted);txt(d,f,a,item->version.empty()?"Unknown":item->version,1290,630,20,white,Align::right);
  txt(d,f,a,"Install Size",761,670,20,muted);txt(d,f,a,item->sizeKnown?uninstaller::bytes(item->size):"Not measured",1290,670,20,white,Align::right);
  txt(d,f,a,"Location",761,707,20,muted);
  std::string path=item->root; if(path.size()>30)path=path.substr(0,28)+"...";
  txt(d,f,a,path,1290,707,17,white,Align::right);
  d.bordered_rect({750,747,562,90},14,color(0x880c25),3,color(0xff4366));
  txt(d,f,a,item->remote||item->kind==uninstaller::Kind::Installed?"Read-only title":"Remove files",1031,806,30,white,Align::center);
  txt(d,f,a,"Save data stays untouched",770,880,20,muted);
  txt(d,f,a,"No system directory is deleted directly",770,926,16,muted);
 }else txt(d,f,a,"Select an application",1025,514,26,muted,Align::center);
 txt(d,f,a,"Console Storage",1400,202,27);
 txt(d,f,a,driveCount>0?driveName+" ("+std::to_string(activeDrive+1)+"/"+
    std::to_string(driveCount)+")   L2/R2":"No reliable volume data",1400,258,17,muted);
 d.line(1379,227,1860,227,1,color(0x46759e));
 if(disk.available&&disk.total){
  unsigned used=(unsigned)(100*(1.0-(double)disk.free/disk.total));
  d.ring(1618,399,104,25,color(0x243c5c));
  d.arc(1618,399,104,25,0.0f,6.2831853f*used/100.0f,color(0x128fff),false);
  txt(d,f,a,std::to_string(used)+"%",1618,411,52,white,Align::center);
  txt(d,f,a,"Used",1618,451,20,muted,Align::center);
  txt(d,f,a,"Total: "+uninstaller::bytes(disk.total),1403,538,22);
  txt(d,f,a,"Used: "+uninstaller::bytes(disk.total-disk.free),1403,585,22);
  txt(d,f,a,"Free: "+uninstaller::bytes(disk.free),1403,631,22);
 }else{
  txt(d,f,a,"Unavailable",1618,414,35,muted,Align::center);
  txt(d,f,a,"No trusted storage telemetry",1618,480,18,muted,Align::center);
 }
 panel(d,1381,691,478,118);
 txt(d,f,a,"Space to be freed (estimated)",1410,734,20,color(0xff8a9c));
 txt(d,f,a,item&&item->sizeKnown?uninstaller::bytes(item->size):"Unknown",1410,787,37,color(0xff5572));
 trash(d,1548,870,1.7f);
 d.bordered_rect({0,1027,1920,53},0,color(0x020d1a),1,color(0x2b496d));
 txt(d,f,a,"D-pad Select    L1/R1 Categories    L2/R2 Storage    X Details    Square Delete    Triangle Refresh    Circle Back",45,1063,17,muted);
 txt(d,f,a,"by YAHBALLON   v0.1.5",1887,1063,18,muted,Align::right);
 if(!msg.empty()){d.rounded_rect({370,944,1170,69},13,color(0x102844));txt(d,f,a,msg.size()>115?msg.substr(0,110)+"...":msg,958,989,19,white,Align::center);}
 if(confirm){
  d.bordered_rect({367,359,1190,322},20,color(0x05162b),3,color(0xff466a));
  txt(d,f,a,"CONFIRM REMOVAL",962,432,39,color(0xff607a),Align::center);
  txt(d,f,a,item?item->name:"Unknown",962,496,25,white,Align::center);
  txt(d,f,a,"X: Delete selected files     Circle: Cancel",962,581,25,white,Align::center);
  txt(d,f,a,"This cannot be undone. Saves are not targeted.",962,636,20,muted,Align::center);
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
 uninstaller::Storage disk{};
 std::vector<uninstaller::bridge::Volume> volumes;
 int selectedDrive=0;
 std::string driveName="Internal SSD";
 auto useDrive=[&](){
   disk={};driveName="Internal SSD";
   if(volumes.empty())return;
   if(selectedDrive>=(int)volumes.size())selectedDrive=0;
   const auto& v=volumes[selectedDrive];
   disk.total=v.total;disk.free=v.free;disk.available=true;
   driveName=v.path=="/user"?"Internal SSD":
      v.path.rfind("/mnt/ext",0)==0?"Extended storage "+v.path:
      "USB storage "+v.path;
 };
 std::map<std::string,std::uint32_t> iconTextures;
 unsigned iconEpoch=0;
 // PS5 statvfs is not proven safe with this runtime; avoid the possible
 // unimplemented libc entry until supported console telemetry is confirmed.
 uninstaller::diag::write("STORAGE","statvfs skipped: PS5 support unverified");
 int tab=0,selected=0,offset=0;bool confirm=false;
 bool firstFrame=true;
 unsigned prev=0;std::string notice="Ready. Installed titles require a verified system uninstall service.";
 uninstaller::diag::write("BOOT","initialization finished; first frame pending");
 for(;;){
  std::array<hui::PadSample,64> s{};unsigned pressed=0,buttons=prev;
  size_t n=pad.read(s);bool connected=false;
  for(size_t i=0;i<n;i++)if(s[i].connected){connected=true;buttons=s[i].buttons;
    if(s[i].left_y<60)buttons|=hui::pad_bits::kUp;
    if(s[i].left_y>195)buttons|=hui::pad_bits::kDown;
    pressed|=buttons&~prev;prev=buttons;
  }
  if(!connected)prev=0;
  if(pressed&hui::pad_bits::kCircle){if(confirm)confirm=false;else{uninstaller::bridge::stop_icons();for(const auto& entry:iconTextures)if(entry.second){GLuint texture=entry.second;glDeleteTextures(1,&texture);}pad.close();renderer.release();display.close();hui::sys::quit();}}
  if(!confirm){
    if(pressed&hui::pad_bits::kL1){tab=(tab+3)%4;selected=offset=0;}
    if(pressed&hui::pad_bits::kR1){tab=(tab+1)%4;selected=offset=0;}
    if(pressed&hui::pad_bits::kL2){
      if(!volumes.empty()){selectedDrive=(selectedDrive+(int)volumes.size()-1)%volumes.size();useDrive();}
    }
    if(pressed&hui::pad_bits::kR2){
      if(!volumes.empty()){selectedDrive=(selectedDrive+1)%volumes.size();useDrive();}
    }
    if(pressed&hui::pad_bits::kLeft){tab=(tab+3)%4;selected=offset=0;}
    if(pressed&hui::pad_bits::kRight){tab=(tab+1)%4;selected=offset=0;}
    if(pressed&hui::pad_bits::kUp){if(selected>0)--selected;if(selected<offset)offset=selected;}
    if(pressed&hui::pad_bits::kDown){if(selected+1<count(items,tab))++selected;if(selected>=offset+11)offset=selected-10;}
    if(pressed&hui::pad_bits::kTriangle){
      uninstaller::diag::write("REFRESH","requested");
      uninstaller::bridge::stop_icons();
      scanRootsReadable=scanRootsMissing=scanRootsDenied=0;
      items=uninstaller::scan(scanLog);
      uninstaller::diag::write("ELFLDR","refresh: launching bundled read-only scanner");
      const auto helper=uninstaller::bridge::scan("/app0/unscan.elf",scanLog);
      uninstaller::bridge::merge(items,helper);
      volumes=helper.volumes;selectedDrive=0;useDrive();
      for(const auto& t:iconTextures)if(t.second){GLuint tex=t.second;glDeleteTextures(1,&tex);}
      iconTextures.clear();++iconEpoch;
      selected=offset=0;
      notice="Entries: "+std::to_string(items.size())+". "+helper.message;
      uninstaller::diag::write("REFRESH","done");
    }
    if(pressed&(hui::pad_bits::kSquare|hui::pad_bits::kCross)){
      const auto* x=nth(items,tab,selected);
      if(x&&x->remote)notice="Read-only helper result. No privileged delete is available.";
      else if(x&&x->kind==uninstaller::Kind::Installed)notice="Installed title uninstall remains disabled.";
      else if(x)confirm=true;
    }
  }else if(pressed&hui::pad_bits::kCross){
    auto* x=nth(items,tab,selected);if(x){
      if(x->remote||x->kind==uninstaller::Kind::Installed){notice="Read-only entry: system uninstall is not enabled.";log("Read-only uninstall blocked: "+x->id);}
      else{
        uninstaller::diag::write("REMOVE_ATTEMPT",x->path.c_str());
        const int result=uninstaller::removeFiles(*x);
        if(result==0){uninstaller::bridge::stop_icons();notice="Selected files removed. Save data not modified.";log("Removed: "+x->path);items=uninstaller::scan(scanLog);const auto r=uninstaller::bridge::scan("/app0/unscan.elf",scanLog);uninstaller::bridge::merge(items,r);volumes=r.volumes;selectedDrive=0;useDrive();selected=offset=0;}
        else{notice="Remove failed: error "+std::to_string(result);log("Remove failed: "+x->path+" error "+std::to_string(result));}
      }
    }
    confirm=false;
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
  DrawList ui;render(ui,font,atlas,items,tab,selected,offset,confirm,notice,disk,iconTextures,driveName,selectedDrive,(int)volumes.size());
  if(firstFrame)uninstaller::diag::write("FIRST_FRAME","draw done");
  renderer.begin();renderer.draw(ui);renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();
  if(firstFrame){
    firstFrame=false;
    uninstaller::diag::write("FIRST_FRAME","present ok");
    uninstaller::diag::write("SCAN","begin after first frame");
    scanRootsReadable=scanRootsMissing=scanRootsDenied=0;
    items=uninstaller::scan(scanLog);
    uninstaller::diag::write("ELFLDR","starting read-only inventory helper via 127.0.0.1:9021");
    uninstaller::bridge::stop_icons();
    const auto helper=uninstaller::bridge::scan("/app0/unscan.elf",scanLog);
    uninstaller::bridge::merge(items,helper);
    volumes=helper.volumes;selectedDrive=0;useDrive();
    uninstaller::diag::write("ELFLDR",(std::string("helper entries=")+std::to_string(helper.items.size())).c_str());
    log("Discovered "+std::to_string(items.size())+" entries; readable roots="+
      std::to_string(scanRootsReadable)+" inaccessible="+
      std::to_string(scanRootsDenied)+" missing="+std::to_string(scanRootsMissing));
    uninstaller::diag::write("SCAN","completed");
    if(items.empty()){
      notice="0 titles. Readable roots: "+std::to_string(scanRootsReadable)+
       ", denied: "+std::to_string(scanRootsDenied)+
       ". "+helper.message;
    }else{
      notice="Found "+std::to_string(items.size())+
       " entries. "+(helper.connected?"elfldr helper connected.":"Direct scan only.");
    }
  }
  hui::sys::sleep_us(16000);
 }
}
