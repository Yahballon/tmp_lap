// SPDX-License-Identifier: GPL-3.0-or-later
// ThemeChanger v0.3.3 - native PS5 Homebrew UI / LiveShell bridge.
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "liveshell_client.hpp"
#include "liveshell_loader.hpp"
#include "motion_catalog.hpp"
#include "theme_log.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include "stb_image.h"
#include <array>
#include <algorithm>
#include <cstdio>
#include <vector>
#include <sys/stat.h>
#include <string>
using namespace hui::gfx;
namespace {
const Color white=Color::rgb(0xf7f9ff), muted=Color::rgb(0xaebbd2);
std::string read(const std::string& path) {
 FILE* f=fopen(path.c_str(),"rb");if(!f)return {};
 if(fseek(f,0,SEEK_END)!=0){fclose(f);return {};}
 long n=ftell(f);rewind(f);if(n<1||n>12000000){fclose(f);return {};}
 std::string s((size_t)n,0);bool ok=fread(s.data(),1,s.size(),f)==s.size();fclose(f);return ok?s:std::string{};
}
unsigned texture(Renderer& r,const std::string& path){
 auto d=read(path);if(d.empty())return 0;
 int w=0,h=0,ch=0;
 unsigned char* p=stbi_load_from_memory((const unsigned char*)d.data(),(int)d.size(),&w,&h,&ch,4);
 if(!p)return 0;unsigned t=w<=2048&&h<=2048?r.batch().create_texture(w,h,p):0;
 stbi_image_free(p);return t;
}
void txt(DrawList& d,const Font& f,unsigned a,const std::string& s,float x,float y,float sz=24,Color c=white,Align align=Align::left){d.text(f,a,s,x,y,sz,c,align);}
void panel(DrawList& d,Rect r,float rad=13,Color fill=Color::rgb(0x040d20),Color border=Color::rgb(0x365074)){d.bordered_rect(r,rad,fill,1.5f,border);}
void symbol(DrawList& d,float x,float y,float size=58){
 d.chamfer_rect({x,y,size,size},size*.23f,Color::rgb(0x069cfb),2,Color::rgb(0x16bafa));
 d.chamfer_rect({x+size*.18f,y+size*.19f,size*.64f,size*.62f},size*.15f,Color::rgb(0x071931),0,{});
 d.rounded_rect({x+size*.48f,y+size*.4f,size*.47f,size*.14f},2,Color::rgb(0x00b9ff));
}
void draw(DrawList& d,const Font& f,unsigned a,const std::array<unsigned,12>& pics,int selected,int active,int page,int opt,const std::string& status,bool live,bool loading,bool sidebar,const motion::Catalog& catalog,
          const std::vector<int>& filtered,const std::vector<unsigned>& thumbnails,
          int onlineSelected,int onlinePage,int filterIndex,int sideRow,
          const std::vector<int>& installedIds,const std::string& activeFile){
 // All positions are in 1920x1080 virtual pixels, derived from the reference's 1671x941 grid.
 // The third row ends at y=985; the footer begins at y=994 (no clipping).
 d.gradient_rect({0,0,1920,1080},0,Color::rgb(0x020a16),Color::rgb(0x010615));
 d.gradient_rect({0,0,375,994},0,Color::rgb(0x061b35),Color::rgb(0x030a18));
 symbol(d,56,27,68);
 txt(d,f,a,"Theme",143,73,32);txt(d,f,a,"Changer",239,73,31,Color::rgb(0x0da4ff));
 txt(d,f,a,"PS5 Homebrew App",150,109,21,muted);
 d.line(2,125,375,125,1,Color::rgb(0x233a57));
 bool side=sidebar&&sideRow==0;
 panel(d,{25,148,325,71},10,Color::rgb(page==0?0x085ef3:0x071b34),Color::rgb(side?0x39c9ff:0x095fba));
 d.bordered_rect({53,170,32,27},5,white,2,white);
 d.circle(76,178,3,Color::rgb(0x075fff));
 d.line(60,190,67,182,3,Color::rgb(0x075fff));d.line(67,182,79,194,3,Color::rgb(0x075fff));
 txt(d,f,a,"Backgrounds",119,194,25);
 bool motionFocus=page==3&&sidebar&&sideRow==1;
 panel(d,{25,235,325,71},10,Color::rgb(page==3?0x085ef3:0x071b34),
       Color::rgb(motionFocus?0x39c9ff:0x1f3857));
 d.bordered_rect({53,254,32,29},5,Color::rgb(0x08254e),2,white);
 d.triangle({63,261,16,17},white,0,1.5708f);
 txt(d,f,a,"Online Wallpapers",119,280,23);
 d.line(29,806,346,806,1,Color::rgb(0x20354d));
 for(int i=1;i<3;i++){
   float y=i==1?833.f:907.f;
   if(page==i||sidebar&&sideRow==i+1)panel(d,{24,y,328,69},10,Color::rgb(0x0a3470),Color::rgb(0x13a0ff));
   d.circle(69,y+33,18,white);txt(d,f,a,i==1?"i":"*",69,y+42,26,Color::rgb(0x041631),Align::center);
   txt(d,f,a,i==1?"About":"Settings",121,y+43,25);
 }
 // The preview is now the same proportions as the user reference (x=440, height=374).
 panel(d,{440,18,1450,375},13,Color::rgb(0x010819),Color::rgb(0x405370));
 unsigned picture=0;
 if(page==0){
   if(selected<12)picture=pics[std::clamp(selected,0,11)];
   else if(selected-12>=0&&selected-12<(int)installedIds.size()){
      int id=installedIds[(size_t)(selected-12)];
      if(id>=0&&id<(int)thumbnails.size())picture=thumbnails[(size_t)id];
   }
 }else if(page!=3)picture=pics[1];
 if(page==3&&!filtered.empty()&&onlineSelected>=0&&onlineSelected<(int)filtered.size()){
   const auto id=filtered[(size_t)onlineSelected];
   if(id>=0&&id<(int)thumbnails.size()&&thumbnails[(size_t)id])picture=thumbnails[(size_t)id];
 }
 if(picture)d.image(picture,{442,119,1446,271},kFullUv,white,10);
 else {
   d.gradient_rect({443,119,1445,270},0,Color::rgb(0x061c42),Color::rgb(0x13447a));
   txt(d,f,a,"Loading wallpaper preview...",1160,343,23,muted,Align::center);
 }
 d.gradient_rect({442,120,1446,218},0,Color::rgb(0x03091c,.68f),Color::rgb(0x01091c,.10f));
 d.gradient_rect({443,20,1443,103},0,Color::rgb(0x010918),Color::rgb(0x020817,.90f));
 txt(d,f,a,(page==0||page==3)?"Preview":page==1?"About":"Settings",474,65,34);
 txt(d,f,a,page==0?"This is how your PS5 menu will look with this theme.":page==3?"MotionBGS: select HD wallpaper and download to LiveShell.":page==1?"ThemeChanger information":"Configure the integrated LiveShell engine",476,99,21,muted);
 txt(d,f,a,"16:42",1857,78,27,white,Align::right);
 // Faux shell tiles are decorative inside the preview and do not pretend to launch installed games.
 for(int i=0;i<9;i++){
  float x=482+i*104.f,y=153;
  if(i==1)d.glow({x,y,92,94},10,13,Color::rgb(0x007fff,.38f));
  panel(d,{x,y,92,91},9,Color::rgb(0x08172e,.95f),Color::rgb(i==1?0x05afff:0x21395b));
  if(i==1)symbol(d,x+17,y+17,57);
  else if(i>=2&&i<6&&pics[(selected+i)%12])
    d.image(pics[(selected+i)%12],{x+3,y+4,86,83},kFullUv,white,8);
  else txt(d,f,a,i==0?"*":(i==6?"[]":(i==7?"O":"#")),x+46,y+60,35,Color::rgb(0xdbeeff),Align::center);
 }
 txt(d,f,a,"Open Theme",586,282,20);
 if(page==0){
  const int total=12+(int)installedIds.size();
  const int start=(selected/12)*12;
  txt(d,f,a,"Available Themes",397,435,30);
  txt(d,f,a,std::to_string(installedIds.size())+" downloaded   |   Page "+
    std::to_string(start/12+1)+" / "+std::to_string((total+11)/12),
    1878,432,18,muted,Align::right);
  for(int pos=0;pos<12&&start+pos<total;pos++){
   const int id=start+pos;
   const bool builtin=id<12;
   int onlineId=-1;std::string name;
   unsigned tex=0;
   if(builtin){name=theme::kNames[id];tex=pics[id];}
   else {
     onlineId=installedIds[(size_t)(id-12)];
     name=catalog.entries()[(size_t)onlineId].title;
     if(onlineId>=0&&onlineId<(int)thumbnails.size())tex=thumbnails[(size_t)onlineId];
   }
   const float x=398+(pos%4)*370.f,y=455+(pos/4)*176.f;
   const float w=348,h=157;
   const bool focus=selected==id&&!sidebar;
   if(focus)d.glow({x,y,w,h},13,13,Color::rgb(0x006eff,.40f));
   panel(d,{x,y,w,h},9,Color::rgb(0x071329),Color::rgb(focus?0x0d9fff:0x324359));
   if(tex)d.image(tex,{x+2,y+2,w-4,h-4},kFullUv,white,7);
   else {
     d.gradient_rect({x+2,y+2,w-4,h-4},8,Color::rgb(0x051c44),Color::rgb(0x134070));
     txt(d,f,a,"Downloaded MP4",x+174,y+77,20,muted,Align::center);
   }
   d.gradient_rect({x+2,y+h-52,w-4,50},4,Color::rgb(0x000000,.01f),Color::rgb(0x000000,.88f));
   if(name.size()>28)name=name.substr(0,25)+"...";
   txt(d,f,a,name,x+15,y+h-17,21,white);
   const bool applied=builtin?id==active:activeFile=="MotionBGS-"+catalog.entries()[(size_t)onlineId].slug+".mp4";
   if(applied){
     d.circle(x+w-33,y+29,20,Color::rgb(0x0876ff));
     txt(d,f,a,"X",x+w-33,y+37,23,white,Align::center);
   }
   if(focus)d.bordered_rect({x-2,y-2,w+4,h+4},10,Color::rgb(0x000000,0),3,Color::rgb(0x04a7ff));
  }
 }else if(page==3){
  const char* categories[]={"All","Featured","Anime","Games","Nature","Cars","Space","Technology","Fantasy"};
  txt(d,f,a,"MotionBGS Online Wallpapers",398,435,30);
  txt(d,f,a,"L2/R2: "+std::string(categories[std::clamp(filterIndex,0,8)])+"   TRIANGLE: next page   X: download / install",
      1880,432,18,muted,Align::right);
  const int begin=onlinePage*12;
  if(filtered.empty())txt(d,f,a,"Online catalog unavailable",1102,644,36,white,Align::center);
  for(int pos=0;pos<12&&begin+pos<(int)filtered.size();pos++){
   int idx=filtered[(size_t)(begin+pos)];
   const auto& item=catalog.entries()[(size_t)idx];
   float x=398+(pos%4)*370.f,y=455+(pos/4)*176.f;
   bool focus=onlineSelected==begin+pos&&!sidebar;
   if(focus)d.glow({x,y,348,159},12,15,Color::rgb(0x006dff,.43f));
   panel(d,{x,y,348,159},10,Color::rgb(0x071b37),Color::rgb(focus?0x00afff:0x334e71));
   if(idx<(int)thumbnails.size()&&thumbnails[(size_t)idx]){
     d.image(thumbnails[(size_t)idx],{x+2,y+2,344,155},kFullUv,white,7);
   }else{
     d.gradient_rect({x+2,y+2,344,155},8,Color::rgb(0x05204a),Color::rgb(0x0e3676));
     d.circle(x+174,y+74,23,Color::rgb(0x1369b0));
     d.triangle({x+169,y+57,27,33},white,0,1.5708f);
   }
   d.gradient_rect({x+2,y+104,344,53},5,Color::rgb(0x000000,0.05f),Color::rgb(0x000000,.87f));
   std::string name=item.title;
   if(name.size()>27)name=name.substr(0,24)+"...";
   txt(d,f,a,name,x+13,y+143,19);
   if(catalog.installed(idx)){
     d.circle(x+324,y+23,16,Color::rgb(0x0bcb8e));
     txt(d,f,a,"X",x+324,y+30,18,white,Align::center);
   }
   if(focus)d.bordered_rect({x-2,y-2,352,163},10,Color::rgb(0x000000,0),3,Color::rgb(0x05a5ff));
  }
  if(catalog.downloading()){
    d.bordered_rect({398,969,1470,13},6,Color::rgb(0x112646),1,Color::rgb(0x2d4669));
    d.rounded_rect({400,972,1466*(catalog.progress()/100.f),7},3,Color::rgb(0x00aaff));
  }
 }else if(page==1){
  panel(d,{437,437,1420,470},13,Color::rgb(0x05132c),Color::rgb(0x254970));
  txt(d,f,a,"ThemeChanger  v0.3.3",490,515,38);
  txt(d,f,a,"Native UI: ps5-homebrew-ui  |  Wallpaper engine: ZiZc3 LiveShell",490,586,24,muted);
  txt(d,f,a,"Includes LiveShell engine v1.1. Starts automatically via elfldr.",490,650,24);
  txt(d,f,a,"Autostart needs jailbreak elfldr on 127.0.0.1:9021.",490,708,24,muted);
  txt(d,f,a,"App log: /data/ThemeChanger/themechanger.log",490,775,24,muted);
 }else{
  panel(d,{437,437,1420,475},13,Color::rgb(0x05132c),Color::rgb(0x264a77));
  const char* fields[]={"Wallpaper engine","Darkness","Pause during games","Hide in system settings","Restart LiveShell engine"};
  for(int i=0;i<5;i++){
   float y=467+i*88;
   panel(d,{466,y,1358,76},10,Color::rgb(i==opt?0x0b397a:0x071a34),Color::rgb(i==opt?0x10a6ff:0x2b4667));
   txt(d,f,a,fields[i],499,y+47,26);
   txt(d,f,a,i==0?"X  Toggle":i==1?"LEFT / RIGHT":i==4?"X  Retry":"X  Toggle",1796,y+47,22,muted,Align::right);
  }
 }
 d.gradient_rect({0,993,1920,87},0,Color::rgb(0x051221,.92f),Color::rgb(0x020816));
 d.line(0,993,1920,993,1,Color::rgb(0x1c3859));
 d.circle(76,1034,16,white);txt(d,f,a,"X",76,1043,24,Color::rgb(0x021329),Align::center);
 txt(d,f,a,"Select",105,1044,22,muted);
 d.circle(225,1034,16,white);txt(d,f,a,"O",225,1043,24,Color::rgb(0x021329),Align::center);
 txt(d,f,a,"Back",256,1044,22,muted);
 txt(d,f,a,"L1/R1: Backgrounds / Online    Square: Retry Engine",403,1045,19,muted);
 txt(d,f,a,loading?"LiveShell starting...":live?"LiveShell connected":"LiveShell offline",1856,1028,19,live?Color::rgb(0x4affbc):Color::rgb(0xffce83),Align::right);
 txt(d,f,a,(page==3?catalog.status():status).substr(0,99),1856,1057,17,muted,Align::right);
}
}
int main(){
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 Renderer renderer;if(!renderer.init())hui::sys::park();
 const std::string title="/data/homebrew/PPSA99891/assets/";
 auto fonts=read("/app0/assets/fonts/inter-semibold.huifont");
 if(fonts.empty())fonts=read(title+"fonts/inter-semibold.huifont");
 Font font;if(!font.load(fonts))hui::sys::park();
 unsigned atlas=renderer.batch().create_font_texture(font);
 std::string asset="/app0/assets";
 if(read(asset+"/wallpapers/ThemeChanger-baseball.png").empty())asset="/data/homebrew/PPSA99891/assets";
 std::array<unsigned,12> pictures{};
 const char* files[]={"baseball","anime","gaming","cyber-city","racing","space","samurai","retro-neon","mountains","forest","beach","snow"};
 for(int i=0;i<12;i++)pictures[i]=texture(renderer,asset+"/wallpapers/ThemeChanger-"+files[i]+".png");
 tc::log("BOOT","ThemeChanger v0.3.2 starting");
 motion::Catalog online;
 online.load(asset);
 std::vector<unsigned> onlineTextures(online.entries().size(),0);
 std::vector<int> installedIds;
 auto refreshInstalled=[&](){
   installedIds.clear();
   for(int i=0;i<(int)online.entries().size();i++)
     if(online.installed(i)) installedIds.push_back(i);
 };
 refreshInstalled();
 tc::log("WALLPAPERS_FOUND",std::to_string(installedIds.size()).c_str());
 std::vector<int> filtered;
 int filterIndex=0, onlineSelected=0, onlinePage=0,sideRow=0;
 const char* categoryKeys[]={"","Featured","Anime","Games","Nature","Cars","Space","Technology","Fantasy"};
 auto rebuildFilter=[&](){
   filtered.clear();
   for(int i=0;i<(int)online.entries().size();i++)
     if(filterIndex==0||online.entries()[(size_t)i].category==categoryKeys[filterIndex])
       filtered.push_back(i);
   onlineSelected=0;onlinePage=0;
 };
 rebuildFilter();
 hui::ps5::Pad pad;pad.open();
 int selected=theme::load_selected(),active=-1,page=0,opt=0,dim=25,frame=0;
 bool sidebar=false;int pendingApply=-1;bool pendingRetry=false;
 const std::string appRoot=asset.substr(0,asset.size()-7);
 engine::start_if_needed(appRoot);
 bool live=theme::probe();if(live)active=theme::current_theme();
 std::string activeFile=live?theme::current_file():std::string{};
 tc::log("LIVESHELL_START",live?"Connected":"Waiting for engine");
 std::string status="Choose a background and press X to apply";
 unsigned previous=0,previousDirections=0;int repeats=0;
 constexpr unsigned arrows=hui::pad_bits::kUp|hui::pad_bits::kDown|
                            hui::pad_bits::kLeft|hui::pad_bits::kRight;
 for(;;){
   std::array<hui::PadSample,64> samples{};const size_t n=pad.read(samples);
   // scePadRead returns a history of samples. OR-ing every sample made a
   // released Cross button look held again, producing repeated APPLY events.
   // Keep the newest connected sample, and preserve state on empty reads.
   unsigned held=previous;
   if(n>0){
     for(size_t i=0;i<n;i++){
       if(!samples[i].connected)continue;
       held=samples[i].buttons;
       if(samples[i].left_x<60)held|=hui::pad_bits::kLeft;
       if(samples[i].left_x>195)held|=hui::pad_bits::kRight;
       if(samples[i].left_y<60)held|=hui::pad_bits::kUp;
       if(samples[i].left_y>195)held|=hui::pad_bits::kDown;
     }
   }
   unsigned pressed=held&~previous;
   const unsigned directions=held&arrows;
   if(directions!=previousDirections){repeats=0;}
   else if(directions){
     ++repeats;
     if(repeats>=20&&((repeats-20)%7)==0)pressed|=directions;
   }else repeats=0;
   previousDirections=directions;
   previous=held;
   if(pressed&hui::pad_bits::kCircle){
     if(page!=0)page=0;
     else{pad.close();renderer.release();display.close();hui::sys::quit();}
   }
   if(pressed&hui::pad_bits::kSquare){engine::start_if_needed(appRoot,true);status="Restart requested: "+engine::status();}
   if(pressed&hui::pad_bits::kL1){page=0;sidebar=false;tc::log("TAB","Backgrounds");}
   if(pressed&hui::pad_bits::kR1){
     page=page==3?0:3;sidebar=false;
     tc::log("TAB",page==3?"Online Wallpapers":"Backgrounds");
   }
   if(page==3&&(pressed&hui::pad_bits::kL2)){filterIndex=(filterIndex+8)%9;rebuildFilter();}
   if(page==3&&(pressed&hui::pad_bits::kR2)){filterIndex=(filterIndex+1)%9;rebuildFilter();}
   if(page==3&&(pressed&hui::pad_bits::kTriangle)&&!filtered.empty()){
     onlinePage=(onlinePage+1)%std::max(1,(int)((filtered.size()+11)/12));
     onlineSelected=std::min((int)filtered.size()-1,onlinePage*12);
   }
   if(page==0||page==3){
     if(sidebar){
       if(pressed&hui::pad_bits::kUp)sideRow=std::max(0,sideRow-1);
       if(pressed&hui::pad_bits::kDown)sideRow=std::min(3,sideRow+1);
       if(pressed&hui::pad_bits::kRight)sidebar=false;
       if(pressed&hui::pad_bits::kCross){
         page=sideRow==0?0:sideRow==1?3:sideRow==2?1:2;sidebar=false;
         tc::log("TAB",(page==3?"Online Wallpapers":page==0?"Backgrounds":page==1?"About":"Settings"));
       }
     }else if(page==3){
       if(pressed&hui::pad_bits::kLeft){
         if(onlineSelected%4==0){sidebar=true;sideRow=1;}
         else onlineSelected=std::max(0,onlineSelected-1);
       }
       if(pressed&hui::pad_bits::kRight)onlineSelected=std::min((int)filtered.size()-1,onlineSelected+1);
       if(pressed&hui::pad_bits::kUp)onlineSelected=std::max(0,onlineSelected-4);
       if(pressed&hui::pad_bits::kDown)onlineSelected=std::min((int)filtered.size()-1,onlineSelected+4);
       if(!filtered.empty())onlinePage=onlineSelected/12;
       if((pressed&hui::pad_bits::kCross)&&!filtered.empty()){
         if(!online.download(filtered[(size_t)onlineSelected]))status="Download already running";
       }
     }else{
     const int total=12+(int)installedIds.size();
     if(pressed&hui::pad_bits::kLeft){
       if(selected%4==0){sidebar=true;sideRow=0;}
       else selected=std::max(0,selected-1);
     }
     if(pressed&hui::pad_bits::kRight)selected=std::min(total-1,selected+1);
     if(pressed&hui::pad_bits::kUp)selected=std::max(0,selected-4);
     if(pressed&hui::pad_bits::kDown)selected=std::min(total-1,selected+4);
     if(pressed&hui::pad_bits::kCross){
       if(selected<12){
         if(theme::install_and_select(selected,asset,status)){
           active=selected;activeFile=theme::theme_file(selected);
           tc::log("SELECT_BUILTIN",activeFile.c_str());
         }else if(!theme::probe()){
           pendingApply=selected;engine::start_if_needed(appRoot,true);
           status="Starting LiveShell. Theme queued...";
         }
       }else if(selected-12<(int)installedIds.size()){
         const auto id=installedIds[(size_t)(selected-12)];
         const auto name="MotionBGS-"+online.entries()[(size_t)id].slug+".mp4";
         if(theme::activate_file(name,status)){
           activeFile=name;active=-1;tc::log("SELECT_DOWNLOADED",name.c_str());
         }else tc::log("SELECT_FAILED",(name+" "+status).c_str());
       }
       live=theme::probe();
     }
     }
   }else if(page==2){
     if(pressed&hui::pad_bits::kUp)opt=std::max(0,opt-1);
     if(pressed&hui::pad_bits::kDown)opt=std::min(4,opt+1);
     if(opt==1&&(pressed&(hui::pad_bits::kLeft|hui::pad_bits::kRight))){
       dim=std::clamp(dim+((pressed&hui::pad_bits::kRight)?5:-5),0,80);
       theme::send_setting("/api/dim?v="+std::to_string(dim),status);
     }
     if(pressed&hui::pad_bits::kCross){
       const char* routes[]={"/api/on","", "/api/set?k=gamepause&v=1","/api/set?k=menuhide&v=1"};
       if(opt!=1)theme::send_setting(routes[opt],status);
       live=theme::probe();
     }
   }
   if(++frame%120==0){
     refreshInstalled();
     selected=std::clamp(selected,0,11+(int)installedIds.size());
     live=theme::probe();
     if(live){
      active=theme::current_theme();
      activeFile=theme::current_file();
      if(pendingApply>=0){int target=pendingApply;pendingApply=-1;
       if(theme::install_and_select(target,asset,status))active=target;}
     }else if(!engine::busy())status=engine::status();
   }
   for(int idx:online.consume_ready()){
     if(idx>=0&&idx<(int)onlineTextures.size()&&!onlineTextures[(size_t)idx]){
       unsigned tex=texture(renderer,online.thumbnail_path(idx));
       if(tex)onlineTextures[(size_t)idx]=tex;
       else tc::log("PREVIEW_DECODE_ERROR",online.entries()[(size_t)idx].slug.c_str());
     }
   }
   if(!online.thumbnail_busy()){
     // Prioritize the focused wallpaper, then fill the visible rows.
     if(page==3&&!filtered.empty()&&onlineSelected<(int)filtered.size()){
       online.request_thumbnail(filtered[(size_t)onlineSelected]);
     }
     if(!online.thumbnail_busy()){
       if(page==3){
         const int first=onlinePage*12;
         for(int q=first;q<(int)filtered.size()&&q<first+12;q++){
           const int id=filtered[(size_t)q];
           if(id>=0&&id<(int)onlineTextures.size()&&!onlineTextures[(size_t)id]&&
               online.request_thumbnail(id))break;
         }
       }else if(page==0){
         const int first=(selected/12)*12;
         for(int q=std::max(0,first-12);q<(int)installedIds.size()&&q<first+12;q++){
           const int id=installedIds[(size_t)q];
           if(id>=0&&id<(int)onlineTextures.size()&&!onlineTextures[(size_t)id]&&
               online.request_thumbnail(id))break;
         }
       }
     }
   }
   DrawList ui;draw(ui,font,atlas,pictures,selected,active,page,opt,status,live,engine::busy(),sidebar,
                     online,filtered,onlineTextures,onlineSelected,onlinePage,filterIndex,sideRow,installedIds,activeFile);
   renderer.begin();renderer.draw(ui);renderer.present(0,display.width(),display.height());display.swap();hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
