// SPDX-License-Identifier: GPL-3.0-or-later
// ThemeChanger v0.1.0 - native PS5 Homebrew UI / LiveShell bridge.
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "liveshell_client.hpp"
#include "liveshell_loader.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"
#include <array>
#include <algorithm>
#include <cstdio>
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
void draw(DrawList& d,const Font& f,unsigned a,const std::array<unsigned,12>& pics,int selected,int active,int page,int opt,const std::string& status,bool live,bool loading,bool sidebar){
 // All positions are in 1920x1080 virtual pixels, derived from the reference's 1671x941 grid.
 // The third row ends at y=985; the footer begins at y=994 (no clipping).
 d.gradient_rect({0,0,1920,1080},0,Color::rgb(0x020a16),Color::rgb(0x010615));
 d.gradient_rect({0,0,375,994},0,Color::rgb(0x061b35),Color::rgb(0x030a18));
 symbol(d,56,27,68);
 txt(d,f,a,"Theme",149,73,39);txt(d,f,a,"Changer",277,73,39,Color::rgb(0x0da4ff));
 txt(d,f,a,"PS5 Homebrew App",150,109,21,muted);
 d.line(2,125,375,125,1,Color::rgb(0x233a57));
 bool side=sidebar&&page==0;
 panel(d,{25,148,325,71},10,Color::rgb(page==0?0x085ef3:0x071b34),Color::rgb(side?0x39c9ff:0x095fba));
 d.bordered_rect({53,170,32,27},5,white,2,white);
 d.circle(76,178,3,Color::rgb(0x075fff));
 d.line(60,190,67,182,3,Color::rgb(0x075fff));d.line(67,182,79,194,3,Color::rgb(0x075fff));
 txt(d,f,a,"Backgrounds",119,194,25);
 d.line(29,806,346,806,1,Color::rgb(0x20354d));
 for(int i=1;i<3;i++){
   float y=i==1?833.f:907.f;
   if(page==i)panel(d,{24,y,328,69},10,Color::rgb(0x0a3470),Color::rgb(0x13a0ff));
   d.circle(69,y+33,18,white);txt(d,f,a,i==1?"i":"*",69,y+42,26,Color::rgb(0x041631),Align::center);
   txt(d,f,a,i==1?"About":"Settings",121,y+43,25);
 }
 // The preview is now the same proportions as the user reference (x=440, height=374).
 panel(d,{440,18,1450,375},13,Color::rgb(0x010819),Color::rgb(0x405370));
 if(pics[selected])d.image(pics[selected],{442,119,1446,271},kFullUv,white,10);
 d.gradient_rect({442,120,1446,218},0,Color::rgb(0x03091c,.68f),Color::rgb(0x01091c,.10f));
 d.gradient_rect({443,20,1443,103},0,Color::rgb(0x010918),Color::rgb(0x020817,.90f));
 txt(d,f,a,page==0?"Preview":page==1?"About":"Settings",474,65,34);
 txt(d,f,a,page==0?"This is how your PS5 menu will look with this theme.":page==1?"ThemeChanger information":"Configure the integrated LiveShell engine",476,99,21,muted);
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
  txt(d,f,a,"Available Themes",397,435,30);
  for(int i=0;i<12;i++){
   const int row=i/4,col=i%4;
   const float x=398+col*370.f, y=455+row*187.f;
   const float w=348, h=159;
   const bool chosen=selected==i&&!sidebar;
   if(chosen)d.glow({x,y,w,h},13,13,Color::rgb(0x006eff,.40f));
   panel(d,{x,y,w,h},9,Color::rgb(0x071329),Color::rgb(chosen?0x0d9fff:0x324359));
   if(pics[i])d.image(pics[i],{x+2,y+2,w-4,h-4},kFullUv,white,7);
   d.gradient_rect({x+2,y+h-56,w-4,54},4,Color::rgb(0x000000,.01f),Color::rgb(0x000000,.88f));
   txt(d,f,a,theme::kNames[i],x+18,y+h-17,22,white);
   if(i==active){d.circle(x+w-34,y+30,20,Color::rgb(0x0876ff));
     txt(d,f,a,"X",x+w-34,y+38,24,white,Align::center);}
   if(chosen)d.bordered_rect({x-2,y-2,w+4,h+4},10,Color::rgb(0x000000,0),3,Color::rgb(0x04a7ff));
  }
 }else if(page==1){
  panel(d,{437,437,1420,470},13,Color::rgb(0x05132c),Color::rgb(0x254970));
  txt(d,f,a,"ThemeChanger  v0.2.0",490,515,38);
  txt(d,f,a,"Native UI: ps5-homebrew-ui  |  Wallpaper engine: ZiZc3 LiveShell",490,586,24,muted);
  txt(d,f,a,"Includes LiveShell engine v1.0. Starts automatically via elfldr.",490,650,24);
  txt(d,f,a,"Autostart needs jailbreak elfldr on 127.0.0.1:9021.",490,708,24,muted);
  txt(d,f,a,"LiveShell logs: /data/LiveShell/liveshell.log",490,775,24,muted);
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
 txt(d,f,a,"Square  Retry Engine",403,1045,20,muted);
 txt(d,f,a,loading?"LiveShell starting...":live?"LiveShell connected":"LiveShell offline",1856,1028,19,live?Color::rgb(0x4affbc):Color::rgb(0xffce83),Align::right);
 txt(d,f,a,status.substr(0,82),1856,1057,17,muted,Align::right);
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
 hui::ps5::Pad pad;pad.open();
 int selected=theme::load_selected(),active=-1,page=0,opt=0,dim=25,frame=0;
 bool sidebar=false;int pendingApply=-1;bool pendingRetry=false;
 const std::string appRoot=asset.substr(0,asset.size()-7);
 engine::start_if_needed(appRoot);
 bool live=theme::probe();if(live)active=theme::current_theme();
 std::string status="Choose a background and press X to apply";
 unsigned previous=0;int repeats=0;
 for(;;){
   std::array<hui::PadSample,64> samples{};const size_t n=pad.read(samples);unsigned pressed=0,held=previous;
   for(size_t i=0;i<n;i++){if(!samples[i].connected)continue;pressed|=samples[i].buttons&~held;held=samples[i].buttons;
      if(samples[i].left_x<60)pressed|=hui::pad_bits::kLeft;
      if(samples[i].left_x>195)pressed|=hui::pad_bits::kRight;
      if(samples[i].left_y<60)pressed|=hui::pad_bits::kUp;
      if(samples[i].left_y>195)pressed|=hui::pad_bits::kDown;
   }
   previous=held;
   if(held&(hui::pad_bits::kUp|hui::pad_bits::kDown|hui::pad_bits::kLeft|hui::pad_bits::kRight)){
     ++repeats;if(repeats>20&&repeats%7==0)pressed|=held&(hui::pad_bits::kUp|hui::pad_bits::kDown|hui::pad_bits::kLeft|hui::pad_bits::kRight);
   }else repeats=0;
   if(pressed&hui::pad_bits::kCircle){
     if(page!=0)page=0;
     else{pad.close();renderer.release();display.close();hui::sys::quit();}
   }
   if(pressed&hui::pad_bits::kSquare){engine::start_if_needed(appRoot,true);status="Restart requested: "+engine::status();}
   if(pressed&hui::pad_bits::kL1){page=0;sidebar=false;}
   if(pressed&hui::pad_bits::kR1){page=(page+1)%3;sidebar=false;}
   if(page==0){
     if(pressed&hui::pad_bits::kLeft)selected=std::max(0,selected-1);
     if(pressed&hui::pad_bits::kRight)selected=std::min(11,selected+1);
     if(pressed&hui::pad_bits::kUp)selected=std::max(0,selected-4);
     if(pressed&hui::pad_bits::kDown)selected=std::min(11,selected+4);
     if(pressed&hui::pad_bits::kCross){
       if(theme::install_and_select(selected,asset,status))active=selected;
       else if(!theme::probe()){pendingApply=selected;engine::start_if_needed(appRoot,true);status="Starting LiveShell. Theme queued...";}
       live=theme::probe();
     }
   }else if(page==2){
     if(pressed&hui::pad_bits::kUp)opt=std::max(0,opt-1);
     if(pressed&hui::pad_bits::kDown)opt=std::min(3,opt+1);
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
     live=theme::probe();
     if(live){
      active=theme::current_theme();
      if(pendingApply>=0){int target=pendingApply;pendingApply=-1;
       if(theme::install_and_select(target,asset,status))active=target;}
     }else if(!engine::busy())status=engine::status();
   }
   DrawList ui;draw(ui,font,atlas,pictures,selected,active,page,opt,status,live,engine::busy(),sidebar);
   renderer.begin();renderer.draw(ui);renderer.present(0,display.width(),display.height());display.swap();hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
