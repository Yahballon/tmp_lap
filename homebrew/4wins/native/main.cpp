// SPDX-License-Identifier: GPL-3.0-or-later
// 4 WINS by YAHBALLON v0.1.0 — native PS5
#include "game.hpp"
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "core/input.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "vendor/stb_image.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <sys/stat.h>
using namespace hui::gfx;
using namespace wins4;
namespace {
constexpr const char* VERSION="4 WINS by YAHBALLON v0.1.0";
constexpr int S=104;
Color rgb(unsigned x,float a=1.f){return Color::rgb(x,a);}
std::string read(const char* name,size_t limit=12*1024*1024){
 FILE* f=fopen(name,"rb");if(!f)return {};
 if(fseek(f,0,SEEK_END)){fclose(f);return {};}
 long n=ftell(f);if(n<0||size_t(n)>limit){fclose(f);return {};}
 rewind(f);std::string data(size_t(n),'\0');if(fread(data.data(),1,data.size(),f)!=data.size())data.clear();
 fclose(f);return data;
}
std::string asset(const char* n,size_t limit=12*1024*1024){
 const char* dirs[]={"/app0/assets/","/data/homebrew/PPSA99858/assets/","assets/"};
 for(auto* dir:dirs){std::string b=read((std::string(dir)+n).c_str(),limit);if(!b.empty())return b;}
 return {};
}
struct Texture{unsigned id=0;};
Texture jpg(Renderer& r,const char* filename){
 auto b=asset(filename);if(b.empty())return {};
 int w=0,h=0,n=0;auto* px=stbi_load_from_memory((const stbi_uc*)b.data(),int(b.size()),&w,&h,&n,4);
 if(!px)return {};unsigned tex=r.batch().create_texture(w,h,px);stbi_image_free(px);return {tex};
}
Texture chip(Renderer& r,const char* filename){
 auto b=asset(filename,S*S*4+1);if(b.size()!=S*S*4)return {};
 return {r.batch().create_texture(S,S,(const uint8_t*)b.data())};
}
void text(DrawList& d,const Font& f,unsigned a,const std::string& s,float x,float y,int size,unsigned clr=0xffffff,Align align=Align::left){
 d.text(f,a,s,x,y,float(size),rgb(clr),align);
}
void panel(DrawList& d,float x,float y,float w,float h,unsigned border=0x22baff){
 d.bordered_rect({x,y,w,h},18,rgb(0x061632,.94f),2,rgb(border));
}
void save(const Game& g){
 mkdir("/data/4WINS",0755);
 FILE* f=fopen("/data/4WINS/settings.tmp","wb");if(!f)return;
 fprintf(f,"mode=%d\nlevel=%d\nfirst=%d\nrumble=%d\nscore1=%d\nscore2=%d\ndraw=%d\n",int(g.opponent),int(g.level),g.first_player_red,g.sound,g.wins[0],g.wins[1],g.draws);
 if(fclose(f)==0)rename("/data/4WINS/settings.tmp","/data/4WINS/settings.ini");
}
void load(Game& g){
 FILE* f=fopen("/data/4WINS/settings.ini","rb");if(!f)return;
 char l[80];int n;
 while(fgets(l,sizeof(l),f)){
  if(sscanf(l,"mode=%d",&n)==1)g.opponent=Opponent(std::clamp(n,0,1));
  else if(sscanf(l,"level=%d",&n)==1)g.level=Level(std::clamp(n,0,2));
  else if(sscanf(l,"first=%d",&n)==1)g.first_player_red=n!=0;
  else if(sscanf(l,"rumble=%d",&n)==1)g.sound=n!=0;
  else if(sscanf(l,"score1=%d",&n)==1)g.wins[0]=std::clamp(n,0,100000);
  else if(sscanf(l,"score2=%d",&n)==1)g.wins[1]=std::clamp(n,0,100000);
  else if(sscanf(l,"draw=%d",&n)==1)g.draws=std::clamp(n,0,100000);
 }
 fclose(f);
}
void draw(DrawList& d,const Game& g,const Font& f,unsigned atlas,const Texture& menu,const Texture& scene,const Texture& red,const Texture& yellow){
 const bool playing=g.screen==Screen::Playing||g.screen==Screen::Paused||g.screen==Screen::Result;
 const auto img=playing?scene.id:menu.id;
 if(img)d.image(img,{0,0,1920,1080},kFullUv,rgb(0xffffff));
 else d.gradient_rect({0,0,1920,1080},0,rgb(0x020817),rgb(0x13255a));
 if(playing){
  // Source game scene is a clean 7 by 6 board; overlay active discs.
  for(int x=0;x<7;x++)for(int r=0;r<6;r++){
    int p=g.at(x,r);unsigned id=p==1?red.id:p==2?yellow.id:0;
    if(id)d.image(id,{573.f+x*112,720.f-r*99,104,104},kFullUv,rgb(0xffffff));
  }
  panel(d,57,334,435,315,g.turn==1?0xff2859:0xffcc40);
  const char* status=g.screen==Screen::Result?(g.winner==1?"RED WINS":g.winner==2?"YELLOW WINS":"DRAW"):g.turn==1?"RED TURN":"YELLOW TURN";
  text(d,f,atlas,status,84,405,34,g.turn==1?0xff506a:0xffdc54);
  text(d,f,atlas,"LEFT/RIGHT   CHOOSE",84,458,25);
  text(d,f,atlas,"X   DROP CHIP",84,507,25);
  text(d,f,atlas,"OPTIONS   PAUSE",84,560,24);
  text(d,f,atlas,"SCORE "+std::to_string(g.wins[0])+" : "+std::to_string(g.wins[1]),84,610,25);
  if(g.screen==Screen::Playing){
    const int x=625+g.selected*112;
    d.ring(float(x),198,24,5,rgb(g.turn==1?0xff3155:0xffdc45));
  }
  if(g.screen==Screen::Result||g.screen==Screen::Paused){
    panel(d,675,400,605,225,0xffdc52);
    text(d,f,atlas,g.screen==Screen::Paused?"GAME PAUSED":status,975,475,42,0xffffff,Align::center);
    text(d,f,atlas,g.screen==Screen::Paused?"X Resume    TRIANGLE Restart":"X Rematch    O Menu",975,551,25,0xa8dcff,Align::center);
  }
 }else if(g.screen==Screen::Home){
  if(g.focus>0)d.bordered_rect({58.f,283.f+float(g.focus)*100,410,76},18,rgb(0x0b3b8f,.65f),4,rgb(0x56ecff));
 }else {
  panel(d,1450,256,410,560);
  const bool mode=g.screen==Screen::Mode;
  const bool diff=g.screen==Screen::Difficulty;
  text(d,f,atlas,mode?"GAME MODE":diff?"BOT DIFFICULTY":g.screen==Screen::Settings?"SETTINGS":"EXIT?",1480,320,28,0x73dcff);
  for(int i=0;i<(mode?2:diff?3:g.screen==Screen::Settings?4:2);i++){
    const std::string val=mode?(i?"VS BOT":"2 PLAYERS"):diff?(i==0?"EASY":i==1?"MEDIUM":"HARD"):g.screen==Screen::Settings?(i==0?"STARTING PLAYER":i==1?"BOT LEVEL":i==2?"RUMBLE":"RESET SCORES"):(i?"NO":"YES");
    const float y=360.f+i*95;
    d.bordered_rect({1470,y,370,76},12,rgb(i==g.submenu?0x124c9b:0x081731,.96f),i==g.submenu?3:1,rgb(i==g.submenu?0x49dbff:0x305478));
    text(d,f,atlas,val,1490,y+49,25);
  }
 }
 text(d,f,atlas,"by YAHBALLON  v0.1.0",1850,1048,20,0xc2d8ff,Align::right);
}
} // namespace
int main(){
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 Renderer renderer;if(!renderer.init())hui::sys::park();
 Font font;auto bytes=read("/app0/assets/fonts/inter-semibold.huifont");
 if(bytes.empty())bytes=read("/data/homebrew/PPSA99858/assets/fonts/inter-semibold.huifont");
 if(!font.load(bytes))hui::sys::park();
 unsigned atlas=renderer.batch().create_font_texture(font);
 Texture home=jpg(renderer,"scene_menu.jpg"),game=jpg(renderer,"scene_game.jpg"),red=chip(renderer,"red.rgba"),yellow=chip(renderer,"yellow.rgba");
 hui::ps5::Pad pad;pad.open();
 Game g;load(g);g.seed^=uint64_t(hui::sys::monotonic_us());
 uint32_t previous=0;uint64_t repeat=0,bot_time=0;bool wait_bot=false;
 for(;;){
  auto now=uint64_t(hui::sys::monotonic_us());
  std::array<hui::PadSample,64> events{};size_t count=pad.read(events);
  uint32_t pressed=0,held=0;bool conn=false;
  for(size_t i=0;i<count;i++){
    if(!events[i].connected)continue;
    conn=true;uint32_t b=events[i].buttons;
    if(events[i].left_x<65)b|=hui::pad_bits::kLeft;if(events[i].left_x>190)b|=hui::pad_bits::kRight;
    if(events[i].left_y<65)b|=hui::pad_bits::kUp;if(events[i].left_y>190)b|=hui::pad_bits::kDown;
    pressed|=b&~previous;previous=b;held=b;
  }
  if(!conn)previous=0;
  uint32_t direction=held&(hui::pad_bits::kLeft|hui::pad_bits::kRight|hui::pad_bits::kUp|hui::pad_bits::kDown);
  uint32_t nav=pressed&direction;
  if(nav)repeat=now+230000;else if(direction&&now>=repeat){nav=direction;repeat=now+120000;}
  auto key=[&](uint32_t bit){return (pressed&bit)!=0;};
  const bool up=(nav&hui::pad_bits::kUp)!=0,down=(nav&hui::pad_bits::kDown)!=0;
  const bool left=(nav&hui::pad_bits::kLeft)!=0,right=(nav&hui::pad_bits::kRight)!=0;
  bool save_now=false;
  switch(g.screen){
   case Screen::Home:
    if(up)g.focus=(g.focus+3)%4;if(down)g.focus=(g.focus+1)%4;
    if(key(hui::pad_bits::kCross)){
     if(g.focus==0){g.reset();wait_bot=false;}
     else if(g.focus==1){g.screen=Screen::Mode;g.submenu=int(g.opponent);}
     else if(g.focus==2){g.screen=Screen::Settings;g.submenu=0;}
     else g.screen=Screen::Exit;
    }if(key(hui::pad_bits::kCircle))g.screen=Screen::Exit;break;
   case Screen::Mode:
    if(up||down)g.submenu=1-g.submenu;
    if(key(hui::pad_bits::kCross)){g.opponent=Opponent(g.submenu);save_now=true;
     if(g.opponent==Opponent::Bot){g.screen=Screen::Difficulty;g.submenu=int(g.level);}else g.screen=Screen::Home;}
    if(key(hui::pad_bits::kCircle))g.screen=Screen::Home;break;
   case Screen::Difficulty:
    if(up)g.submenu=(g.submenu+2)%3;if(down)g.submenu=(g.submenu+1)%3;
    if(key(hui::pad_bits::kCross)){g.level=Level(g.submenu);g.screen=Screen::Home;save_now=true;}
    if(key(hui::pad_bits::kCircle))g.screen=Screen::Mode;break;
   case Screen::Settings:
    if(up)g.submenu=(g.submenu+3)%4;if(down)g.submenu=(g.submenu+1)%4;
    if(left||right||key(hui::pad_bits::kCross)){
     if(g.submenu==0)g.first_player_red=!g.first_player_red;
     if(g.submenu==1)g.level=Level((int(g.level)+1)%3);
     if(g.submenu==2)g.sound=!g.sound;
     if(g.submenu==3){g.wins[0]=g.wins[1]=g.draws=0;}
     save_now=true;
    }
    if(key(hui::pad_bits::kCircle))g.screen=Screen::Home;break;
   case Screen::Playing:
    if(key(hui::pad_bits::kOptions)){g.screen=Screen::Paused;break;}
    if(g.opponent==Opponent::Bot&&g.turn==2){
     if(!wait_bot){bot_time=now+400000;wait_bot=true;}
     if(now>=bot_time){int m=g.choose_bot();if(m>=0){g.selected=m;g.drop(m);save_now=g.screen==Screen::Result;}wait_bot=false;}
     break;
    }
    if(left)g.selected=(g.selected+6)%7;if(right)g.selected=(g.selected+1)%7;
    if(key(hui::pad_bits::kCross)||(g.opponent==Opponent::Human&&g.turn==2&&key(hui::pad_bits::kCircle))){
     if(g.drop(g.selected)){save_now=g.screen==Screen::Result;wait_bot=false;if(g.sound)pad.rumble(.13f,.1f);}
    }break;
   case Screen::Paused:
    if(key(hui::pad_bits::kCross)||key(hui::pad_bits::kOptions))g.screen=Screen::Playing;
    if(key(hui::pad_bits::kTriangle)){g.reset();wait_bot=false;}
    if(key(hui::pad_bits::kCircle))g.screen=Screen::Home;break;
   case Screen::Result:
    if(key(hui::pad_bits::kCross)){g.reset();wait_bot=false;}
    if(key(hui::pad_bits::kCircle))g.screen=Screen::Home;break;
   case Screen::Exit:
    if(key(hui::pad_bits::kCross)){
      save(g);pad.close();if(home.id)glDeleteTextures(1,&home.id);if(game.id)glDeleteTextures(1,&game.id);
      if(red.id)glDeleteTextures(1,&red.id);if(yellow.id)glDeleteTextures(1,&yellow.id);
      renderer.release();display.close();hui::sys::quit();
    }if(key(hui::pad_bits::kCircle))g.screen=Screen::Home;break;
  }
  if(save_now)save(g);
  DrawList d;draw(d,g,font,atlas,home,game,red,yellow);
  renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();pad.tick(.016f);hui::sys::sleep_us(16000);
 }
}
