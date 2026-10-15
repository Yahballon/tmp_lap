// SPDX-License-Identifier: GPL-3.0-or-later
// ThemeChanger v0.1.0 - native PS5 Homebrew UI / LiveShell bridge.
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "liveshell_client.hpp"
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
void draw(DrawList& d,const Font& f,unsigned a,const std::array<unsigned,12>& pictures,int selected,int active,int page,int opt,const std::string& status,bool live){
 d.gradient_rect({0,0,1920,1080},0,Color::rgb(0x020916),Color::rgb(0x00040c));
 d.gradient_rect({0,0,380,1080},0,Color::rgb(0x07172e),Color::rgb(0x030915));
 symbol(d,51,28,69);
 txt(d,f,a,"Theme",145,77,40);txt(d,f,a,"Changer",283,77,40,Color::rgb(0x0799ff));
 txt(d,f,a,"PS5 Homebrew App",149,115,22,muted);
 d.line(0,130,373,130,1,Color::rgb(0x1f3654));
 panel(d,{24,153,328,72},12,Color::rgb(page==0?0x075bf0:0x071b34),Color::rgb(page==0?0x038eff:0x162e50));
 txt(d,f,a,"Backgrounds",112,198,27);
 d.circle(65,188,13,white);d.circle(65,188,6,Color::rgb(0x075bf0));
 d.line(29,844,351,844,1,Color::rgb(0x1b344e));
 for(int i=1;i<3;i++){
  const float y=869+(i-1)*76.f;bool focus=page==i;
  if(focus)panel(d,{24,y-10,327,67},12,Color::rgb(0x0a3677),Color::rgb(0x148fff));
  d.circle(69,y+24,15,white);txt(d,f,a,i==1?"i":"*",69,y+32,21,Color::rgb(0x052249),Align::center);
  txt(d,f,a,i==1?"About":"Settings",119,y+32,25);
 }
 // Screenshot-scaled preview panel (the mock OS row is intentionally illustrative).
 panel(d,{410,22,1480,405},14,Color::rgb(0x020b1b),Color::rgb(0x3b4a63));
 if(pictures[selected])d.image(pictures[selected],{412,124,1476,300},kFullUv,white,10);
 d.gradient_rect({411,23,1477,106},0,Color::rgb(0x020714,0.99f),Color::rgb(0x020714,0.78f));
 txt(d,f,a,page==0?"Preview":(page==1?"About":"Settings"),447,78,38);
 txt(d,f,a,page==0?"This is how your PS5 menu will look with this theme.":(page==1?"ThemeChanger for PS5 homebrew":"LiveShell wallpaper preferences"),448,110,22,muted);
 // Mock home-screen cards: these are not actual PS5 app tiles.
 for(int i=0;i<9;i++){
   float x=449+i*100.f;
   panel(d,{x,161,88,93},10,Color::rgb(i==1?0x010a17:0x071b3d,0.88f),Color::rgb(i==1?0x03b8ff:0x164370));
   if(i==1)symbol(d,x+18,177,52);
   else if(i>1&&i<6&&pictures[(selected+i)%12])d.image(pictures[(selected+i)%12],{x+3,164,82,87},kFullUv,white,8);
   else txt(d,f,a,i==0?"*":(i==6?"[]":(i==7?"O":"#")),x+43,217,36,Color::rgb(0xb1d7f7),Align::center);
 }
 txt(d,f,a,"Open Theme",552,290,20);
 txt(d,f,a,"16:42",1858,80,27,white,Align::right);
 if(page==0){
  txt(d,f,a,"Available Themes",410,480,30);
  for(int i=0;i<12;i++){
    int col=i%4,row=i/4;float x=408+col*377.f,y=500+row*200.f;bool current=selected==i;
    if(current)d.glow({x,y,357,183},16,17,Color::rgb(0x058cff,.42f));
    panel(d,{x,y,356,181},12,Color::rgb(0x071224),Color::rgb(current?0x10b4ff:0x384a65));
    if(pictures[i])d.image(pictures[i],{x+3,y+3,350,174},kFullUv,white,10);
    d.gradient_rect({x+3,y+127,350,51},8,Color::rgb(0x000000,0.08f),Color::rgb(0x000000,.9f));
    txt(d,f,a,theme::kNames[i],x+18,y+166,23);
    if(i==active){d.circle(x+330,y+30,23,Color::rgb(0x006aff));txt(d,f,a,"X",x+330,y+38,23,white,Align::center);}
    if(current)d.bordered_rect({x-2,y-2,360,185},13,Color::rgb(0x000000,0),3,Color::rgb(0x05a9ff));
  }
 }else if(page==1){
  panel(d,{438,501,1420,380});txt(d,f,a,"ThemeChanger v0.1.0",485,571,35);
  txt(d,f,a,"Native UI: ps5-homebrew-ui by BlackBearReloaded (GPL-3.0-or-later)",485,632,25,muted);
  txt(d,f,a,"Wallpaper engine: LiveShell by ZiZc3 (GPL-3.0)",485,685,25,muted);
  txt(d,f,a,"LiveShell payload must be loaded before backgrounds can be applied.",485,738,25,muted);
 }else{
  panel(d,{438,496,1420,415});
  const char* labels[]={"Enable wallpaper","Darkness","Pause during games","Hide in system settings"};
  for(int i=0;i<4;i++){
    float y=518+i*87.f;
    panel(d,{465,y,1365,73},9,Color::rgb(i==opt?0x0c3268:0x07182e),Color::rgb(i==opt?0x169eff:0x213958));
    txt(d,f,a,labels[i],500,y+46,27);
    txt(d,f,a,i==0?"X  Toggle":i==1?"LEFT / RIGHT  Adjust":(i==2?"X  Toggle":"X  Toggle"),1789,y+46,22,muted,Align::right);
  }
 }
 d.gradient_rect({0,1017,1920,63},0,Color::rgb(0x020a17),Color::rgb(0x010711));
 txt(d,f,a,"X   Select",63,1056,24);txt(d,f,a,"O   Back",226,1056,24);
 txt(d,f,a,live?"LiveShell connected":"LiveShell offline",1882,1038,18,live?Color::rgb(0x2ce8b6):Color::rgb(0xffb874),Align::right);
 txt(d,f,a,status.substr(0,108),1882,1063,17,muted,Align::right);
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
   if(pressed&hui::pad_bits::kL1)page=0;
   if(pressed&hui::pad_bits::kR1)page=(page+1)%3;
   if(page==0){
     if(pressed&hui::pad_bits::kLeft)selected=std::max(0,selected-1);
     if(pressed&hui::pad_bits::kRight)selected=std::min(11,selected+1);
     if(pressed&hui::pad_bits::kUp)selected=std::max(0,selected-4);
     if(pressed&hui::pad_bits::kDown)selected=std::min(11,selected+4);
     if(pressed&hui::pad_bits::kCross){
       if(theme::install_and_select(selected,asset,status))active=selected;
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
   if(++frame%360==0){live=theme::probe();if(live)active=theme::current_theme();}
   DrawList ui;draw(ui,font,atlas,pictures,selected,active,page,opt,status,live);
   renderer.begin();renderer.draw(ui);renderer.present(0,display.width(),display.height());display.swap();hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
