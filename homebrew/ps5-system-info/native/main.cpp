// SPDX-License-Identifier: GPL-3.0-or-later
// PS5 SYSTEM INFO by YAHBALLON v0.1.0 - read-only dashboard.
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "core/input.hpp"
#include "data.hpp"
#include <array>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
#include <cstdint>
using namespace hui::gfx;
namespace {
const Color white=Color::rgb(0xf0f7ff),muted=Color::rgb(0xabc5e5),edge=Color::rgb(0x1459a5),green=Color::rgb(0x48e998);
std::string readFile(const char* path){
 FILE* f=fopen(path,"rb");if(!f)return {};
 if(fseek(f,0,SEEK_END)!=0){fclose(f);return {};}
 long n=ftell(f);if(n<1||n>16000000){fclose(f);return {};}
 rewind(f);std::string d((size_t)n,0);const size_t k=fread(d.data(),1,d.size(),f);
 fclose(f);if(k!=d.size())return {};return d;
}
void tx(DrawList& d,const Font& f,unsigned a,const std::string& s,float x,float y,int size=19,Color c=white,Align align=Align::left){
 d.text(f,a,s,x,y,(float)size,c,align);
}
std::string shorten(const std::string& s,size_t len){return s.size()>len?s.substr(0,len>3?len-3:0)+"...":s;}
void box(DrawList& d,float x,float y,float w,float h,bool focus=false){
 d.bordered_rect({x,y,w,h},12,Color::rgb(focus?0x102e63:0x04142b),focus?2.5f:1,focus?Color::rgb(0x3d9bff):edge);
}
void hdr(DrawList& d,const Font& f,unsigned a,const std::string& s,float x,float y,float w){
 d.rounded_rect({x+1,y+1,w-2,42},11,Color::rgb(0x071e43));
 d.circle(x+24,y+22,9,Color::rgb(0x147efe));
 tx(d,f,a,s,x+44,y+28,22);
}
void rows(DrawList& d,const Font& f,unsigned a,const std::vector<info::Row>& rows_,
 float x,float top,float width,float maxY,int scroll=0,bool showUnavailable=true,int textSize=17,int spacing=34){
 int i=0;
 for(const auto& r:rows_){
  if(!showUnavailable&&!r.available)continue;
  float y=top+i*spacing-scroll;
  if(y>top-11&&y<maxY){
   tx(d,f,a,shorten(r.label,34),x,y,textSize,muted);
   tx(d,f,a,shorten(r.value,width>600?62:34),x+width-12,y,textSize,r.available?white:Color::rgb(0x6583a5),Align::right);
   d.line(x,y+10,x+width,y+10,1,Color::rgb(0x0e315a));
  }
  ++i;
 }
 if(i==0)tx(d,f,a,"No available readings in this section",x,top+16,20,muted);
}
const char* pages[]={"Overview","System","Hardware","Storage","Network","Display / Video",
 "Audio","Power & Thermals","Peripherals","Installed Apps","Processes","Kernel / Debug","Logs","Settings"};
constexpr int count=14;
const char* sections[]={"Console Information","Hardware","Storage","Network","Video / Display","Audio","Installed Apps","Peripherals","Processes","Power & Fan"};
std::string formatAge(long long s){
 char b[80];snprintf(b,sizeof(b),"%lldh %02lldm %02llds",s/3600,(s/60)%60,s%60);return b;
}
void overview(DrawList& d,const Font& f,unsigned a,const info::Snapshot& s,bool show){
 // Overview follows screenshot columns; cards are read-only and navigable by their sidebar category.
 box(d,286,91,555,380);hdr(d,f,a,"Console Information",286,91,555);
 rows(d,f,a,s.system,303,163,519,457,0,show,16,30);
 box(d,851,91,491,380);hdr(d,f,a,"Hardware",851,91,491);
 rows(d,f,a,s.hardware,869,163,456,457,0,show,16,30);
 box(d,1352,91,540,380);hdr(d,f,a,"Console Profile",1352,91,540);
 d.circle(1622,255,102,Color::rgb(0x082b69));
 d.bordered_rect({1556,177,131,169},22,Color::rgb(0x112c60),3,Color::rgb(0x37a6fc));
 d.rounded_rect({1581,204,75,111},12,Color::rgb(0x1d5592));
 for(int i=0;i<7;i++)d.line(1576+i*6,219,1576+i*6,314,1,Color::rgb(0x5cbcff,.34f));
 tx(d,f,a,"PS5",1623,394,37,white,Align::center);
 tx(d,f,a,"Illustration / not device identification",1623,427,16,muted,Align::center);
 box(d,286,482,490,284);hdr(d,f,a,"Storage",286,482,490);
 rows(d,f,a,s.storage,304,547,452,752,0,show,16,31);
 box(d,786,482,555,284);hdr(d,f,a,"Network",786,482,555);
 rows(d,f,a,s.network,804,547,515,752,0,show,16,30);
 box(d,1352,482,540,284);hdr(d,f,a,"Video / Display",1352,482,540);
 rows(d,f,a,s.video,1369,547,502,751,0,show,16,31);
 box(d,286,776,490,248);hdr(d,f,a,"Installed Apps",286,776,490);
 tx(d,f,a,std::to_string(s.homebrewExecutables)+" homebrew executables",309,865,27);
 tx(d,f,a,"Detected in /data/homebrew",309,907,19,muted);
 tx(d,f,a,"Press X on Installed Apps for full list",309,960,17,muted);
 box(d,786,776,555,248);hdr(d,f,a,"Peripherals",786,776,555);
 rows(d,f,a,s.peripherals,806,840,512,1009,0,show,16,31);
 box(d,1352,776,540,248);hdr(d,f,a,"Power & Fan",1352,776,540);
 rows(d,f,a,s.power,1369,840,499,1008,0,show,16,31);
}
const std::vector<info::Row>* pageRows(const info::Snapshot& s,int page){
 switch(page){
 case 1:return &s.system;case 2:return &s.hardware;case 3:return &s.storage;
 case 4:return &s.network;case 5:return &s.video;case 6:return &s.audio;
 case 7:return &s.power;case 8:return &s.peripherals;case 10:return &s.processes;
 case 11:return &s.kernel;case 12:return &s.logs;default:return nullptr;
 }
}
void side(DrawList& d,const Font& f,unsigned a,int selected){
 box(d,13,90,255,902);
 for(int i=0;i<count;i++){
  const float y=105+i*62.5f;
  if(selected==i){
   d.shadow({22,y,236,51},14,12,Color::rgb(0x1679ff,.32f));
   d.bordered_rect({22,y,236,51},10,Color::rgb(0x074de1),2,Color::rgb(0x3ac6ff));
  }
  const int group=i%5;
  if(group==0)d.rounded_rect({36,y+14,19,19},4,Color::rgb(selected==i?0xffffff:0x629ce8));
  else if(group==1)d.circle(45,y+24,10,Color::rgb(selected==i?0xffffff:0x629ce8));
  else if(group==2){d.line(35,y+15,54,y+15,3,Color::rgb(0x6ca8ef));d.line(35,y+29,54,y+29,3,Color::rgb(0x6ca8ef));}
  else if(group==3)d.bordered_rect({35,y+13,21,23},4,Color::rgb(0x09234d),2,Color::rgb(0x6ca8ef));
  else d.circle(45,y+24,10,Color::rgb(0x6ca8ef));
  tx(d,f,a,pages[i],72,y+32,18,selected==i?white:muted);
 }
}
}
int main(){
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 hui::gfx::Renderer renderer;if(!renderer.init())hui::sys::park();
 Font font;auto fontData=readFile("/app0/assets/fonts/inter-semibold.huifont");
 if(fontData.empty())fontData=readFile("/data/homebrew/PPSA99786/assets/fonts/inter-semibold.huifont");
 if(!font.load(fontData))hui::sys::park();
 unsigned atlas=renderer.batch().create_font_texture(font);
 hui::ps5::Pad pad;pad.open();
 hui::InputTracker tracker;
 info::Settings settings=info::loadSettings();
 info::Snapshot snap=info::collect(display.width(),display.height(),false);
 info::appendLog("BOOT PS5 System Info v0.1.0");
 std::string notice="Read-only diagnostics. Unavailable fields have no verified API.";
 int page=0,scroll=0,appSelected=0,settingsSelected=0;
 long long lastSample=(long long)hui::sys::monotonic_us();
 for(;;){
  std::array<hui::PadSample,64> samples{};size_t n=pad.read(samples);
  auto input=tracker.update(std::span<const hui::PadSample>(samples.data(),n),(std::uint64_t)hui::sys::monotonic_us());
  bool update=false;
  if(input.nav==hui::Direction::up){
   if(page==9&&appSelected>0)appSelected--;
   else if(page==13&&settingsSelected>0)settingsSelected--;
   else if(page!=0)scroll=std::max(0,scroll-46);
   else page=(page+count-1)%count;
  }
  if(input.nav==hui::Direction::down){
   if(page==9)appSelected=std::min((int)snap.apps.size()-1,appSelected+1);
   else if(page==13)settingsSelected=std::min(2,settingsSelected+1);
   else if(page!=0)scroll+=46;
   else page=(page+1)%count;
  }
  if(input.nav==hui::Direction::left){page=(page+count-1)%count;scroll=0;}
  if(input.nav==hui::Direction::right){page=(page+1)%count;scroll=0;}
  if(input.is_pressed(hui::Action::page_prev)){page=(page+count-1)%count;scroll=0;}
  if(input.is_pressed(hui::Action::page_next)){page=(page+1)%count;scroll=0;}
  if(input.is_pressed(hui::Action::menu)){page=13;scroll=0;}
  if(input.is_pressed(hui::Action::north)){update=true;notice="Manual diagnostics refresh requested";}
  if(input.is_pressed(hui::Action::back)){
   if(page!=0){page=0;scroll=0;}
   else{pad.close();renderer.release();display.close();hui::sys::quit();}
  }
  if(input.is_pressed(hui::Action::confirm)){
   if(page==0){page=1;scroll=0;}
   else if(page==13){
    if(settingsSelected==0){const int opts[]={0,5,10,30,60};int index=0;for(int i=0;i<5;i++)if(opts[i]==settings.refreshSeconds)index=i;settings.refreshSeconds=opts[(index+1)%5];}
    if(settingsSelected==1)settings.showUnavailable=!settings.showUnavailable;
    if(settingsSelected==2)settings.theme=1-settings.theme;
    notice=info::saveSettings(settings)?"Settings saved":"Could not save settings";update=true;
   }else if(page==12){info::appendLog("User-requested diagnostic log marker");update=true;notice="Log marker saved";}
   else update=true;
  }
  if(input.is_pressed(hui::Action::west)){
   std::string path;
   notice=info::exportReport(snap,settings,path)?"Report exported: "+path:"Cannot write report to /data/ps5-system-info";
  }
  const auto now=(long long)hui::sys::monotonic_us();
  if(update||(settings.refreshSeconds>0 && now-lastSample>=settings.refreshSeconds*1000000ll)){
   snap=info::collect(display.width(),display.height(),input.connected);
   lastSample=now;
  }
  const auto* rr=pageRows(snap,page);
  if(rr){
   int visible=0;for(const auto& r:*rr)if(settings.showUnavailable||r.available)visible++;
   scroll=std::clamp(scroll,0,std::max(0,(visible*52)-760));
  }else scroll=0;
  DrawList d;
  d.gradient_rect({0,0,1920,1080},0,Color::rgb(settings.theme?0x101521:0x010918),Color::rgb(settings.theme?0x101c31:0x041536));
  d.line(200,1040,1670,24,1,Color::rgb(0x073b89,.45f));
  d.arc(1672,204,615,2.5f,3.2f,6.1f,Color::rgb(0x006eea,.19f));
  d.rounded_rect({0,0,1920,79},0,Color::rgb(0x041027));
  d.bordered_rect({34,14,48,48},8,Color::rgb(0x0645a0),3,Color::rgb(0x41a7ff));
  for(int i=0;i<4;i++){d.line(41+i*11,8,41+i*11,14,3,Color::rgb(0x53b7ff));d.line(41+i*11,62,41+i*11,68,3,Color::rgb(0x53b7ff));}
  tx(d,font,atlas,"PS5 SYSTEM INFO  by YAHBALLON",106,42,33,white);
  tx(d,font,atlas,"Detailed read-only information about your console",109,66,17,muted);
  tx(d,font,atlas,"v0.1.0",1887,42,21,white,Align::right);
  side(d,font,atlas,page);
  if(page==0)overview(d,font,atlas,snap,settings.showUnavailable);
  else if(rr){
   box(d,286,91,1605,933);hdr(d,font,atlas,pages[page],286,91,1605);
   tx(d,font,atlas,"Only confirmed readings are displayed as live measurements",315,167,20,muted);
   d.push_clip({308,183,1550,803});
   rows(d,font,atlas,*rr,323,240,1498,983,scroll,settings.showUnavailable,22,52);
   d.pop_clip();
   int rendered=0;for(const auto& r:*rr)rendered+=settings.showUnavailable||r.available;
   tx(d,font,atlas,std::to_string(rendered)+" fields  |  D-Pad up/down scroll  |  L1/R1 change section",324,1005,17,muted);
  }else if(page==9){
   box(d,286,91,1605,933);hdr(d,font,atlas,"Installed Apps",286,91,1605);
   tx(d,font,atlas,std::to_string(snap.homebrewExecutables)+" homebrew applications discovered in /data/homebrew",323,167,21,muted);
   tx(d,font,atlas,"Does not include protected retail games or installed system apps.",323,202,18,muted);
   const int start=std::max(0,appSelected-10);
   for(int i=start;i<(int)snap.apps.size()&&i<start+11;i++){
    const float y=227+(i-start)*69;
    box(d,319,y,1543,61,i==appSelected);
    tx(d,font,atlas,shorten(snap.apps[i].title,53),340,y+37,24);
    tx(d,font,atlas,snap.apps[i].id,1290,y+37,20,muted,Align::right);
    tx(d,font,atlas,info::compactBytes(snap.apps[i].bytes)+" executable",1833,y+37,19,muted,Align::right);
   }
   if(snap.apps.empty())tx(d,font,atlas,"No readable homebrew executables found.",330,313,26,muted);
  }else if(page==13){
   box(d,286,91,1605,933);hdr(d,font,atlas,"Settings",286,91,1605);
   const char* labels[]={"Automatic refresh interval","Show unavailable fields","Interface theme"};
   const std::string values[]={settings.refreshSeconds?std::to_string(settings.refreshSeconds)+" seconds":"Manual only",
     settings.showUnavailable?"On":"Off",settings.theme?"Slate":"Midnight blue"};
   const char* desc[]={"Sampling is read-only and does not query private kernel offsets",
    "Keep all screenshot categories visible even when metrics cannot be measured",
    "Switch the dashboard color palette"};
   for(int i=0;i<3;i++){
    const float y=210+i*156;
    box(d,320,y,1534,131,i==settingsSelected);
    tx(d,font,atlas,labels[i],351,y+48,27);
    tx(d,font,atlas,values[i],1820,y+48,27,Color::rgb(0x50b4ff),Align::right);
    tx(d,font,atlas,desc[i],352,y+89,18,muted);
   }
   tx(d,font,atlas,"Report: /data/ps5-system-info/report.txt",339,762,22,white);
   tx(d,font,atlas,"Log: /data/ps5-system-info/system-info.log",339,806,21,muted);
   tx(d,font,atlas,"Saved settings: /data/ps5-system-info/settings.ini",339,850,21,muted);
   tx(d,font,atlas,"X Change selected setting  |  SQUARE Export report",339,972,20,muted);
  }
  d.rounded_rect({0,1031,1920,49},0,Color::rgb(0x07182e));
  tx(d,font,atlas,shorten(notice,115),22,1061,17,muted);
  tx(d,font,atlas,"D-Pad Navigate  |  L1/R1 Sections  |  X Select  |  TRIANGLE Refresh  |  SQUARE Export  |  OPTIONS Settings  |  O Back",1893,1062,16,white,Align::right);
  renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
