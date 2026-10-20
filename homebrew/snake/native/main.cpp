// SPDX-License-Identifier: GPL-3.0-or-later
// SNAKE by YAHBALLON v0.1.0 | Native PS5 OpenGL game.
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
using namespace snake;
namespace {
constexpr float BX=647.f,BY=190.f,CELL=30.f,BS=720.f;
const Color WHITE=Color::rgb(0xe5fff0),MUTED=Color::rgb(0x9de5c2);
Color cc(uint32_t rgb,float alpha=1){return Color::rgb(rgb,alpha);}
void text(DrawList& d,const Font& font,unsigned atlas,const std::string& s,float x,float y,float size,Color c=WHITE,Align a=Align::left){
 d.text(font,atlas,s,x,y,size,c,a);
}
std::string read_file(const char* name){
 FILE* f=fopen(name,"rb");if(!f)return {};
 if(fseek(f,0,SEEK_END)){fclose(f);return {};}
 long len=ftell(f);if(len<1||len>15000000){fclose(f);return {};}
 rewind(f);std::string bytes(size_t(len),'\0');
 if(fread(bytes.data(),1,bytes.size(),f)!=bytes.size())bytes.clear();
 fclose(f);return bytes;
}
GLuint load_bg(Renderer& renderer){
 std::string bytes=read_file("/app0/assets/jungle.jpg");
 if(bytes.empty())bytes=read_file("/data/homebrew/PPSA99846/assets/jungle.jpg");
 if(bytes.empty())return 0;
 int w=0,h=0,n=0;
 auto* pixels=stbi_load_from_memory((const stbi_uc*)bytes.data(),int(bytes.size()),&w,&h,&n,4);
 if(!pixels||w<100||h<100||w>4096||h>4096){if(pixels)stbi_image_free(pixels);return 0;}
 GLuint handle=renderer.batch().create_texture(w,h,pixels);
 stbi_image_free(pixels);return handle;
}
void ensure_dir(){mkdir("/data/SnakeGame",0755);}
void log(const char* msg){
 ensure_dir();if(FILE* f=fopen("/data/SnakeGame/game.log","ab")){
  fprintf(f,"[Snake v0.1.0] %s\n",msg);fclose(f);
 }
}
void save(const Game& g){
 ensure_dir();
 FILE* f=fopen("/data/SnakeGame/save.tmp","wb");
 if(!f){log("save open failed");return;}
 fprintf(f,"v=1\nhigh=%d\nspeed=%d\nwalls=%d\nfood=%d\nmode=%d\n",
  g.high_score,g.speed,int(g.walls),g.food_type,int(g.mode));
 if(fclose(f)==0 && rename("/data/SnakeGame/save.tmp","/data/SnakeGame/save.ini")!=0)log("save rename failed");
}
void load(Game& g){
 FILE* f=fopen("/data/SnakeGame/save.ini","rb");if(!f)return;
 char buf[128];bool valid=false;int n=0;
 while(fgets(buf,sizeof(buf),f)){
  if(sscanf(buf,"v=%d",&n)==1){valid=(n==1);continue;}
  if(!valid)continue;
  if(sscanf(buf,"high=%d",&n)==1)g.high_score=std::clamp(n,0,99999999);
  else if(sscanf(buf,"speed=%d",&n)==1)g.set_speed(n);
  else if(sscanf(buf,"walls=%d",&n)==1)g.walls=(n!=0);
  else if(sscanf(buf,"food=%d",&n)==1)g.set_food(n);
  else if(sscanf(buf,"mode=%d",&n)==1)g.set_mode(n);
 }
 fclose(f);
}
void panel(DrawList& d,float x,float y,float w,float h,bool selected=false){
 if(selected)d.glow({x,y,w,h},12,12,cc(0x29ff35,.60f));
 d.bordered_rect({x,y,w,h},14,selected?cc(0x0a4630,.94f):cc(0x03161b,.96f),
                  selected?3:2,selected?cc(0x5cff44):cc(0x184540));
}
void small_panel(DrawList& d,const Font& f,unsigned a,int index,bool focus){
 const float y=451.f+index*142.f;
 panel(d,1489,y,381,122,focus);
 const char* labels[]={"SPEED","WALLS","FOOD TYPE"};
 text(d,f,a,labels[index],1516,y+39,24,cc(0x60e78b));
 if(index<2){text(d,f,a,"<",1528,y+94,35,WHITE);}
}
void board_frame(DrawList& d,const Font& f,unsigned a,const Game& g){
 d.shadow({594,45,848,922},28,20,cc(0x000000,.9f));
 d.bordered_rect({601,47,839,907},31,cc(0x081a1b,.94f),6,cc(0x143f35));
 d.bordered_rect({621,67,799,866},21,cc(0x020f10),5,cc(0x55ff5c));
 d.glow({631,84,780,840},15,16,cc(0x4bff4f,.13f));
 d.bordered_rect({635,80,770,847},14,cc(0x010b0d),2,cc(0x124f34));
 text(d,f,a,"SCORE",669,130,23,cc(0x79d8a0));
 text(d,f,a,"HI-SCORE",1366,130,23,cc(0x79d8a0),Align::right);
 char buffer[24];
 snprintf(buffer,sizeof(buffer),"%06d",g.score);
 text(d,f,a,buffer,669,166,30,WHITE);
 snprintf(buffer,sizeof(buffer),"%06d",g.high_score);
 text(d,f,a,buffer,1366,166,30,WHITE,Align::right);
 d.bordered_rect({BX-2,BY-2,BS+4,BS+4},10,cc(0x010e0c),2,cc(0x20a94d));
 // Grid is live geometry, not baked into the backdrop.
 for(int x=1;x<WIDTH;x++)d.line(BX+x*CELL,BY,BX+x*CELL,BY+BS,1,cc(0x0f5845,.54f));
 for(int y=1;y<HEIGHT;y++)d.line(BX,BY+y*CELL,BX+BS,BY+y*CELL,1,cc(0x0f5845,.54f));
}
void pixel(DrawList& d,float x,float y,float size,uint32_t rgb){
 d.rounded_rect({x+2,y+2,size-4,size-4},3,cc(rgb));
 d.rounded_rect({x+4,y+4,size-8,5},1,cc(0xc8ff8a,.52f));
 d.line(x+4,y+size-5,x+size-4,y+size-5,2,cc(0x126d22));
}
void draw_food(DrawList& d,const Game& g,float x,float y,float size){
 auto base=g.food_type==2?0xffb300:g.food_type==1?0xbe26e8:0xff2f48;
 d.circle(x+size*.5f,y+size*.58f,size*.34f,cc(base,.40f));
 d.rounded_rect({x+size*.17f,y+size*.29f,size*.7f,size*.64f},5,cc(base));
 d.rounded_rect({x+size*.23f,y+size*.36f,size*.15f,size*.16f},2,cc(0xffc2c3));
 d.rounded_rect({x+size*.49f,y+size*.14f,4,size*.20f},1,cc(0x5d7b20));
 d.rounded_rect({x+size*.54f,y+size*.15f,size*.25f,size*.11f},2,cc(0x55f347));
}
void board(DrawList& d,const Game& g){
 draw_food(d,g,BX+CELL*g.food.x,BY+CELL*g.food.y,CELL);
 for(int i=int(g.body.size())-1;i>=0;i--){
  const auto p=g.body[size_t(i)];float x=BX+p.x*CELL,y=BY+p.y*CELL;
  pixel(d,x,y,CELL,i==0?0x6eff1b:(i%3==0?0x33df15:0x55ec14));
 }
 if(g.body.empty())return;
 const auto h=g.body.front();
 float x=BX+h.x*CELL,y=BY+h.y*CELL;
 const float ex=g.direction==Dir::Left?7:g.direction==Dir::Right?19:9;
 const float ey=g.direction==Dir::Up?9:g.direction==Dir::Down?20:12;
 d.rounded_rect({x+ex-3,y+ey-3,6,7},1,cc(0x003b16));
 d.rounded_rect({x+(g.direction==Dir::Up||g.direction==Dir::Down?19:ex)-3,
                 y+(g.direction==Dir::Up||g.direction==Dir::Down?ey:20)-3,6,7},1,cc(0x003b16));
 if(g.direction==Dir::Right)d.line(x+27,y+19,x+35,y+19,3,cc(0xff4059));
 if(g.direction==Dir::Left)d.line(x+3,y+19,x-5,y+19,3,cc(0xff4059));
}
void button(DrawList& d,const Font& f,unsigned a,int index,int focus){
 const float y=407+102*index;
 const bool active=index==focus;
 panel(d,106,y,444,88,active);
 static const char* labels[]={"Play Game","Game Mode","Settings","High Scores","Exit"};
 static const char* glyphs[]={">","[+]","*","#",">"};
 text(d,f,a,glyphs[index],146,y+56,39,active?WHITE:cc(0xc2f3da));
 text(d,f,a,labels[index],224,y+55,28,WHITE);
}
void sidebar(DrawList& d,const Font& f,unsigned a,const Game& g,int sideFocus){
 panel(d,1468,436,421,472);
 for(int i=0;i<3;i++)small_panel(d,f,a,i,sideFocus==i);
 text(d,f,a,std::to_string(g.speed),1670,545,31,cc(0xb7ff8f),Align::center);
 text(d,f,a,">",1818,545,37,MUTED,Align::center);
 for(int i=0;i<8;i++)
  d.rounded_rect({1516+i*32.f,570,25,19},1,cc(i<g.speed?0x5dfc50:0x123e30));
 text(d,f,a,g.walls?"ON":"OFF",1674,681,30,WHITE,Align::center);
 for(int yy=0;yy<5;yy++)for(int xx=0;xx<5;xx++){
  bool edge=xx==0||xx==4||yy==0||yy==4;
  d.rounded_rect({1788.f+xx*9.f,650.f+yy*9.f,7,7},0,cc(edge?0x75d38f:0x204737));
 }
 text(d,f,a,g.food_type==0?"Apple":g.food_type==1?"Cherry":"Golden",1670,826,28,WHITE,Align::center);
 draw_food(d,g,1784,796,58);
}
void footer(DrawList& d,const Font& f,unsigned a,const Game& g){
 d.rounded_rect({520,966,780,84},10,cc(0x001110,.91f));
 d.circle(722,1009,21,cc(0x328cee));
 d.ring(722,1009,21,3,cc(0x84c9ff));
 text(d,f,a,"X",722,1018,26,WHITE,Align::center);
 text(d,f,a,"Select",762,1020,24,WHITE);
 d.ring(888,1009,20,4,cc(0xff3d69));
 text(d,f,a,"O",888,1018,22,cc(0xff7e98),Align::center);
 text(d,f,a,"Back",925,1020,24,WHITE);
 d.bordered_rect({1030,987,42,42},10,cc(0x27142d),3,cc(0xd34eff));
 text(d,f,a,"[]",1051,1017,18,cc(0xff97fc),Align::center);
 text(d,f,a,g.page==Page::Game?"Pause":"Start Game",1085,1020,24,WHITE);
}
void overlay(DrawList& d,const Font& f,unsigned a,const Game& g){
 if(g.page==Page::Home){
  for(int i=0;i<5;i++)button(d,f,a,i,g.focus);
  return;
 }
 if(g.page==Page::Game){
  text(d,f,a,"SNAKE RUNNING",112,474,29,cc(0x9dfb80));
  text(d,f,a,"D-PAD / LEFT STICK",112,551,26,WHITE);
  text(d,f,a,"OPTIONS  Pause",112,619,23,MUTED);
  text(d,f,a,"Food eaten: "+std::to_string(g.apples),112,697,25,WHITE);
  text(d,f,a,"X  Pause",112,775,24,MUTED);
  return;
 }
 if(g.page==Page::Paused||g.page==Page::GameOver||g.page==Page::Exit||
    g.page==Page::Modes||g.page==Page::Settings||g.page==Page::Scores){
  d.shadow({692,285,622,474},24,20,cc(0x000000,.9f));
  d.bordered_rect({694,284,620,480},18,cc(0x031917,.99f),3,cc(0x54e957));
  d.line(724,378,1284,378,2,cc(0x3d9846));
 }
 auto heading=[&](const char* s){text(d,f,a,s,742,360,39,cc(0x7cff60));};
 if(g.page==Page::Paused){
  heading("GAME PAUSED");
  text(d,f,a,"X / OPTIONS   Resume",745,459,28);
  text(d,f,a,"TRIANGLE     New Game",745,528,27);
  text(d,f,a,"O                  Main Menu",745,598,27);
 }else if(g.page==Page::GameOver){
  heading(g.won?"YOU WIN!":"GAME OVER");
  text(d,f,a,"Score: "+std::to_string(g.score),745,469,32);
  if(g.new_record)text(d,f,a,"NEW HIGH SCORE!",745,520,26,cc(0xffc92d));
  text(d,f,a,"X    Try Again",745,612,28);
  text(d,f,a,"O    Main Menu",745,676,27);
 }else if(g.page==Page::Modes){
  heading("GAME MODE");
  const char* names[]={"CLASSIC","ZEN","SPRINT"};
  const char* descriptions[]={"Walls & body collision","No collisions, endless play","Faster movement and reflexes"};
  for(int i=0;i<3;i++){
   float y=411+i*105;
   d.bordered_rect({736,y,529,85},12,cc(g.mode_focus==i?0x135b31:0x0a2824),2,cc(g.mode_focus==i?0x77ff4f:0x205541));
   text(d,f,a,names[i],762,y+35,26,WHITE);
   text(d,f,a,descriptions[i],762,y+64,18,MUTED);
   if(int(g.mode)==i)text(d,f,a,"ACTIVE",1228,y+36,18,cc(0x7afa55),Align::right);
  }
  text(d,f,a,"X  Select Mode      O  Back",760,717,21,MUTED);
 }else if(g.page==Page::Settings){
  heading("SETTINGS");
  const char* labels[]={"Snake Speed","Solid Walls","Food Type"};
  const std::string values[]={std::to_string(g.speed)+" / 8",g.walls?"ON":"OFF",g.food_type==0?"Apple":g.food_type==1?"Cherry":"Golden"};
  for(int i=0;i<3;i++){
   float y=414+i*96;
   d.bordered_rect({731,y,540,77},10,cc(g.settings_focus==i?0x154d30:0x0b2924),2,cc(g.settings_focus==i?0x80ff52:0x2a5543));
   text(d,f,a,labels[i],757,y+48,25);
   text(d,f,a,values[i],1239,y+48,24,cc(0xc2ff81),Align::right);
  }
  text(d,f,a,"LEFT / RIGHT Adjust    O Back",755,729,21,MUTED);
 }else if(g.page==Page::Scores){
  heading("HIGH SCORES");
  text(d,f,a,"BEST SCORE",773,467,26,MUTED);
  text(d,f,a,std::to_string(g.high_score),1244,487,50,cc(0xb7ff81),Align::right);
  text(d,f,a,"Stored locally on this console",773,584,21,MUTED);
  text(d,f,a,"O  Back",773,686,24);
 }else if(g.page==Page::Exit){
  heading("EXIT SNAKE?");
  text(d,f,a,"X  Quit to homebrew launcher",747,489,27);
  text(d,f,a,"O  Cancel",747,585,27);
 }
}
} // namespace

int main(){
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 hui::gfx::Renderer renderer;if(!renderer.init())hui::sys::park();
 Font font;std::string font_data=read_file("/app0/assets/fonts/inter-semibold.huifont");
 if(font_data.empty())font_data=read_file("/data/homebrew/PPSA99846/assets/fonts/inter-semibold.huifont");
 if(!font.load(font_data))hui::sys::park();
 unsigned atlas=renderer.batch().create_font_texture(font);
 GLuint jungle=load_bg(renderer);
 hui::ps5::Pad pad;pad.open();
 Game g;load(g);g.seed(uint64_t(hui::sys::monotonic_us())^0xdead4411ULL);
 log("Boot");uint32_t old_buttons=0;uint64_t repeat_at=0;
 int side_focus=-1;uint64_t prev=uint64_t(hui::sys::monotonic_us());
 for(;;){
  uint64_t now=uint64_t(hui::sys::monotonic_us());
  float dt=std::clamp(float(now-prev)/1000000.f,0.f,.08f);prev=now;
  std::array<hui::PadSample,64> events{};size_t count=pad.read(events);
  uint32_t pressed=0,held=0;bool connected=false;
  for(size_t i=0;i<count;i++){
   if(!events[i].connected)continue;
   connected=true;uint32_t bits=events[i].buttons;
   if(events[i].left_y<65)bits|=hui::pad_bits::kUp;
   if(events[i].left_y>190)bits|=hui::pad_bits::kDown;
   if(events[i].left_x<65)bits|=hui::pad_bits::kLeft;
   if(events[i].left_x>190)bits|=hui::pad_bits::kRight;
   pressed|=bits&~old_buttons;old_buttons=bits;held=bits;
  }
  if(!connected){old_buttons=0;held=0;}
  const uint32_t directions=held&(hui::pad_bits::kUp|hui::pad_bits::kDown|hui::pad_bits::kLeft|hui::pad_bits::kRight);
  uint32_t nav=pressed&directions;
  if(nav)repeat_at=now+230000;
  else if(directions&&now>=repeat_at){nav=directions;repeat_at=now+120000;}
  else if(!directions)repeat_at=0;
  auto pressed_key=[&](uint32_t key){return (pressed&key)!=0;};
  bool up=bool(nav&hui::pad_bits::kUp),down=bool(nav&hui::pad_bits::kDown);
  bool left=bool(nav&hui::pad_bits::kLeft),right=bool(nav&hui::pad_bits::kRight);
  bool persist=false;
  if(g.page==Page::Home){
   if(side_focus<0){
    if(up)g.focus=(g.focus+4)%5;
    if(down)g.focus=(g.focus+1)%5;
    if(right)side_focus=0;
    if(pressed_key(hui::pad_bits::kSquare)){g.start();log("Game started");}
    if(pressed_key(hui::pad_bits::kCross)){
     switch(g.focus){
      case 0:g.start();log("Game started");break;
      case 1:g.page=Page::Modes;g.mode_focus=int(g.mode);break;
      case 2:g.page=Page::Settings;g.settings_focus=0;break;
      case 3:g.page=Page::Scores;break;
      case 4:g.page=Page::Exit;break;
     }
    }
   }else{
    if(up)side_focus=(side_focus+2)%3;
    if(down)side_focus=(side_focus+1)%3;
    if(left&&!pressed_key(hui::pad_bits::kCross))side_focus=-1;
    if(right||pressed_key(hui::pad_bits::kCross)){
     if(side_focus==0)g.set_speed(g.speed==8?1:g.speed+1);
     else if(side_focus==1)g.walls=!g.walls;
     else g.set_food(g.food_type+1);
     persist=true;
    }
   }
   if(pressed_key(hui::pad_bits::kCircle)){if(side_focus>=0)side_focus=-1;else g.page=Page::Exit;}
  }else if(g.page==Page::Game){
   // Direction uses only newly pressed keys: no held-input autoreversal.
   if(pressed_key(hui::pad_bits::kUp)||up)g.turn(Dir::Up);
   else if(pressed_key(hui::pad_bits::kDown)||down)g.turn(Dir::Down);
   else if(pressed_key(hui::pad_bits::kLeft)||left)g.turn(Dir::Left);
   else if(pressed_key(hui::pad_bits::kRight)||right)g.turn(Dir::Right);
   if(pressed_key(hui::pad_bits::kOptions)||pressed_key(hui::pad_bits::kCross))g.toggle_pause();
   if(pressed_key(hui::pad_bits::kCircle))g.toggle_pause();
   g.tick(dt);
   if(g.changed){persist=true;g.changed=false;}
   if(g.page==Page::GameOver){persist=true;log("Game over");pad.rumble(.2f,.24f);}
  }else if(g.page==Page::Paused){
   if(pressed_key(hui::pad_bits::kCross)||pressed_key(hui::pad_bits::kOptions))g.toggle_pause();
   if(pressed_key(hui::pad_bits::kTriangle))g.start();
   if(pressed_key(hui::pad_bits::kCircle)){g.page=Page::Home;side_focus=-1;persist=true;}
  }else if(g.page==Page::GameOver){
   if(pressed_key(hui::pad_bits::kCross))g.start();
   if(pressed_key(hui::pad_bits::kCircle)){g.page=Page::Home;side_focus=-1;}
  }else if(g.page==Page::Modes){
   if(up)g.mode_focus=(g.mode_focus+2)%3;
   if(down)g.mode_focus=(g.mode_focus+1)%3;
   if(pressed_key(hui::pad_bits::kCross)){g.set_mode(g.mode_focus);persist=true;g.page=Page::Home;g.focus=1;}
   if(pressed_key(hui::pad_bits::kCircle)){g.page=Page::Home;g.focus=1;}
  }else if(g.page==Page::Settings){
   if(up)g.settings_focus=(g.settings_focus+2)%3;
   if(down)g.settings_focus=(g.settings_focus+1)%3;
   if(left||right||pressed_key(hui::pad_bits::kCross)){
    int delta=left?-1:1;
    if(g.settings_focus==0)g.set_speed(g.speed+delta);
    else if(g.settings_focus==1)g.walls=!g.walls;
    else g.set_food(g.food_type+delta);
    persist=true;
   }
   if(pressed_key(hui::pad_bits::kCircle)){g.page=Page::Home;g.focus=2;}
  }else if(g.page==Page::Scores){
   if(pressed_key(hui::pad_bits::kCircle)||pressed_key(hui::pad_bits::kCross)){g.page=Page::Home;g.focus=3;}
  }else if(g.page==Page::Exit){
   if(pressed_key(hui::pad_bits::kCross)){
    save(g);log("Exit");pad.close();
    if(jungle)glDeleteTextures(1,&jungle);
    renderer.release();display.close();hui::sys::quit();
   }
   if(pressed_key(hui::pad_bits::kCircle)){g.page=Page::Home;g.focus=4;}
  }
  if(pressed_key(hui::pad_bits::kOptions)&&g.page==Page::Home){g.page=Page::Settings;g.settings_focus=0;}
  if(persist)save(g);
  pad.tick(dt);
  DrawList d;
  if(jungle)d.image(jungle,{0,0,1920,1080},kFullUv,WHITE);
  else{
   d.gradient_rect({0,0,1920,1080},0,cc(0x001a18),cc(0x051108));
   text(d,font,atlas,"SNAKE",55,258,122,cc(0x7fff4b));
   text(d,font,atlas,"BY YAHBALLON",179,308,28,WHITE);
  }
  board_frame(d,font,atlas,g);
  board(d,g);
  sidebar(d,font,atlas,g,side_focus);
  overlay(d,font,atlas,g);
  footer(d,font,atlas,g);
  renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
