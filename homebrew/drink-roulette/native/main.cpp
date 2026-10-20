// SPDX-License-Identifier: GPL-3.0-or-later
// Drink Roulette by YAHBALLON v0.1.1 — native PS5 / ps5-homebrew-ui.
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
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include <sys/stat.h>

using namespace hui::gfx;
using namespace roulette;
namespace {
const Color white=Color::rgb(0xfaf6ff), light=Color::rgb(0xd3d7e9),muted=Color::rgb(0xa3a4b6);
Color rgb(uint32_t value,float alpha=1.f){return Color::rgb(value,alpha);}
std::string read(const char* path,size_t maxBytes=12*1024*1024){
 FILE* f=fopen(path,"rb");if(!f)return {};
 if(fseek(f,0,SEEK_END)){fclose(f);return {};}
 long n=ftell(f);if(n<=0||size_t(n)>maxBytes){fclose(f);return {};}
 rewind(f);std::string content(size_t(n),'\0');
 if(fread(content.data(),1,size_t(n),f)!=size_t(n))content.clear();fclose(f);return content;
}
void log(const char* text){mkdir("/data/DrinkRoulette",0755);if(FILE* f=fopen("/data/DrinkRoulette/drink-roulette.log","ab")){fprintf(f,"[DrinkRoulette v0.1.1] %s\n",text);fclose(f);}}
void save(const Game& g){
 mkdir("/data/DrinkRoulette",0755);
 FILE* f=fopen("/data/DrinkRoulette/settings.tmp","wb");if(!f)return;
 fprintf(f,"v=1\nplayers=%d\nmode=%d\nrounds=%d\n",g.player_count,g.mode,g.rounds);
 for(int i=0;i<8;i++)fprintf(f,"name%d=%s\n",i,g.players[i].name.c_str());
 if(fclose(f)==0)rename("/data/DrinkRoulette/settings.tmp","/data/DrinkRoulette/settings.ini");
}
void load(Game& g){
 FILE* f=fopen("/data/DrinkRoulette/settings.ini","rb");if(!f)return;
 char line[256]={};bool ok=false;
 while(fgets(line,sizeof(line),f)){
  int n=0;
  if(sscanf(line,"v=%d",&n)==1){ok=(n==1);continue;}
  if(!ok)continue;
  if(sscanf(line,"players=%d",&n)==1)g.player_count=std::clamp(n,2,8);
  else if(sscanf(line,"mode=%d",&n)==1)g.mode=std::clamp(n,0,1);
  else if(sscanf(line,"rounds=%d",&n)==1&&n>=0&&n<=30)g.rounds=n;
  else{
   int i=-1;char name[100]={};
   if(sscanf(line,"name%d=%99[^\n]",&i,name)==2&&i>=0&&i<8){
    // Save format only admits names from this preset list; reject arbitrary input.
    for(auto allowed:name_pool)if(std::string(name)==allowed)g.players[i].name=allowed;
   }
  }
 }
 fclose(f);
}
void tx(DrawList& d,const Font& f,unsigned a,const std::string& s,float x,float y,float size,Color c=white,Align align=Align::left){d.text(f,a,s,x,y,size,c,align);}
void panel(DrawList& d,float x,float y,float w,float h,uint32_t border=0x37435c,float alpha=.96f){
 d.shadow({x,y,w,h},18,15,rgb(0x01010d,.65f));
 d.bordered_rect({x,y,w,h},18,rgb(0x090e1d,alpha),1.7f,rgb(border,.82f));
}
struct Texture {GLuint id=0;int width=0,height=0;bool reference=false;};
Texture background(Renderer& r){
 Texture result;
 std::string bytes=read("/app0/assets/menu_background.jpg");
 if(bytes.empty())bytes=read("/data/homebrew/PPSA99841/assets/menu_background.jpg");
 result.reference=!bytes.empty();
 if(bytes.empty())bytes=read("/app0/assets/generated_scene.jpg");
 if(bytes.empty())bytes=read("/data/homebrew/PPSA99841/assets/generated_scene.jpg");
 if(bytes.empty())return result;
 int w=0,h=0,n=0;
 auto* p=stbi_load_from_memory((const stbi_uc*)bytes.data(),int(bytes.size()),&w,&h,&n,4);
 if(!p||w<1||h<1||w>4096||h>4096){if(p)stbi_image_free(p);return result;}
 result.id=r.batch().create_texture(w,h,p);result.width=w;result.height=h;
 stbi_image_free(p);return result;
}
// A dark, warm, original party-room fallback when a photo asset isn't installed.
void party_backdrop(DrawList& d){
 d.gradient_rect({0,0,1920,1080},0,rgb(0x020713),rgb(0x3b1729));
 d.rounded_rect({720,40,635,470},120,rgb(0x0b0e22,.75f));
 d.rounded_rect({1110,142,590,240},58,rgb(0x1a1228,.96f));
 d.rounded_rect({1170,195,480,160},30,rgb(0x262034));
 d.rounded_rect({1260,256,165,66},20,rgb(0x382638));
 d.rounded_rect({1470,254,170,66},20,rgb(0x224038));
 const float tableShape[]={0,540,1920,410,1920,1080,0,1080};d.polygon(tableShape,4,rgb(0x2e1b1a));
 for(int i=0;i<21;i++){
  float x=25.f+i*96.f, y=18+std::sin(i*.8f)*33;
  d.circle(x,y,21,rgb(0xffac39,.09f));
  d.circle(x,y,10,rgb(0xffbc65,.18f));
  d.circle(x,y,4,rgb(0xffdf90,.94f));
  if(i>0)d.line(x-96,y-4,x,y,2,rgb(0x785148,.75f));
 }
 for(int i=0;i<10;i++){
  float x=1120.f+i*90.f, y=475.f+(i%3)*38;
  d.circle(x,y,25,rgb(i%2?0xfa5d1a:0xea266c,.15f));
 }
 // Cups and bottles behind the roulette table.
 for(int i=0;i<9;i++){
  float x=1400.f+float((i*77)%420),y=470+float(i%4)*80;
  d.rounded_rect({x,y,46,58},6,rgb(0x8d151d,.8f));
  d.line(x+5,y+6,x+40,y+6,5,rgb(0xe75d4e));
 }
 d.rounded_rect({0,975,1920,105},0,rgb(0x070b16,.98f));
}
void title(DrawList& d,const Font& f,unsigned a){
 tx(d,f,a,"Drink",165,168,83,rgb(0xff2997));tx(d,f,a,"Roulette",438,168,83,rgb(0x36e6ff));
 tx(d,f,a,"Drinking Game",357,238,45,rgb(0xffb535));
 d.line(358,250,725,250,4,rgb(0xff914a));
 tx(d,f,a,"GOOD FRIENDS",1210,62,26,rgb(0x2aabff));
 tx(d,f,a,"BAD DECISIONS",1204,97,25,rgb(0xff3ea9));
 tx(d,f,a,"PARTY MODE ON",1601,87,27,rgb(0xffa02a));
}
void menu_button(DrawList& d,const Font& f,unsigned a,int i,const char* name,const char* symbol,bool focus){
 const float y=309+float(i)*98.f;
 if(focus)d.glow({89,y,490,82},16,20,rgb(0xe329ff,.8f));
 d.bordered_rect({92,y,490,80},15,focus?rgb(0x640b92,.98f):rgb(0x0a1124,.96f),focus?3.f:1.4f,focus?rgb(0xf13eff):rgb(0x303d57));
 tx(d,f,a,symbol,125,y+54,34,focus?rgb(0xffdd61):light);
 tx(d,f,a,name,207,y+52,30,white);tx(d,f,a,">",550,y+52,30,focus?rgb(0xff8ffb):muted,Align::right);
}
void wheel(DrawList& d,const Font& f,unsigned a,const Game& g){
 constexpr float cx=996,cy=631,r=349;
 d.shadow({cx-r-17,cy-r+26,2*(r+17),2*(r+17)},350,32,rgb(0x000000,.82f));
 d.circle(cx,cy+10,r+25,rgb(0x090707));
 d.ring(cx,cy+9,r+23,10,rgb(0xa66b33));
 d.circle(cx,cy,r+10,rgb(0x060815));
 const float sliceWidth=tau/10.f;
 for(int i=0;i<10;i++){
  float start=(i-.5f)*sliceWidth;
  float sweep=sliceWidth-.015f;
  Color c=rgb(slices[i].color);
  d.arc(cx,cy,r,175,start,sweep,c,false);
  d.arc(cx,cy,r+2,5,start+.017f,sweep-.032f,rgb(0xffffff,.8f),false);
  float rad=i*sliceWidth;
  float x=cx+std::sin(rad)*270, y=cy-std::cos(rad)*270;
  std::string name=slices[i].label;
  if(name=="GROUP DRINK"){
   tx(d,f,a,"GROUP",x,y-8,17,white,Align::center);
   tx(d,f,a,"DRINK",x,y+15,17,white,Align::center);
  }else{
   tx(d,f,a,name,x,y,19,white,Align::center);
  }
 }
 d.ring(cx,cy,r+9,5,rgb(0xe4b96e));
 d.circle(cx,cy,179,rgb(0x1c1418));
 d.ring(cx,cy,178,4,rgb(0x795534));
 d.circle(cx,cy,162,rgb(0x100d12));
 // Radial woodgrain/depth in the fixed inner disc.
 for(int i=0;i<10;i++){float a=i*tau/10;d.line(cx+std::sin(a)*22,cy-std::cos(a)*22,cx+std::sin(a)*160,cy-std::cos(a)*160,2,rgb(0xb17d47,.12f));}
 // Bottle pointer built from native polygons; moves each frame rather than being pre-rendered.
 const float angle=g.angle;
 const float dx=std::sin(angle),dy=-std::cos(angle),nx=-dy,ny=dx;
 auto p=[&](float l,float w,int n)->std::array<float,2>{(void)n;return {cx+dx*l+nx*w,cy+dy*l+ny*w};};
  // Outline closed separately to correctly taper the neck and body.
 const std::array<std::array<float,2>,16> bottle{{
 {-185,-9},{-171,-12},{-158,-18},{-144,-34},{-116,-40},
 {5,-40},{62,-34},{87,-13},{162,-13},{174,-10},
 {174,10},{162,13},{87,13},{62,34},{5,40},{-116,40}
 }};
 std::vector<float> xy;xy.reserve(36);
 for(auto pt:bottle){auto z=p(pt[0],pt[1],0);xy.push_back(z[0]);xy.push_back(z[1]);}
 // Bottle base is a solid green silhouette. A last polygon forms the rounded heel.
 d.polygon(xy.data(),int(xy.size()/2),rgb(0x038c36));
 auto top=p(-80,-30,0),end=p(28,-30,0);
 d.line(top[0],top[1],end[0],end[1],7,rgb(0x75ff85,.65f));
 top=p(-89,26,0);end=p(20,26,0);
 d.line(top[0],top[1],end[0],end[1],6,rgb(0x01291b,.8f));
 auto a1=p(57,-13,0),b1=p(157,-13,0);
 d.line(a1[0],a1[1],b1[0],b1[1],4,rgb(0x67ff9a,.88f));
 auto a2=p(-36,-32,0),b2=p(-36,32,0);d.line(a2[0],a2[1],b2[0],b2[1],42,rgb(0xe7bd5a,.59f));
 auto a3=p(-36,-14,0),b3=p(-36,14,0);d.line(a3[0],a3[1],b3[0],b3[1],3,rgb(0xfffca3,.5f));
 const auto tip=p(179,0,0);d.circle(tip[0],tip[1],9,rgb(0x27ff6d));
 d.circle(cx,cy,11,rgb(0xffe29b));d.circle(cx,cy,4,rgb(0xffffff));
 // Highlight currently selected sector with a small fixed needle at the bottle tip.
 if(g.spinning){d.ring(cx,cy,r+14,3,rgb(0xffffff,.25f));}
}
void cards(DrawList& d,const Font& f,unsigned a,const Game& g){
 panel(d,1428,116,415,344);
 tx(d,f,a,"Players ("+std::to_string(g.player_count)+")",1454,163,25);
 d.line(1454,180,1818,180,1,rgb(0x647088,.4f));
 const uint32_t pc[]={0xff3c72,0x32b5ff,0x42ef9b,0xffaa36,0xd088ff,0xffe54f,0x40e9e6,0xff6e53};
 for(int i=0;i<g.player_count&&i<4;i++){
  float y=209+i*56;
  d.circle(1466,y+10,20,rgb(pc[i]));
  tx(d,f,a,"P"+std::to_string(i+1),1496,y+16,18,rgb(pc[i]));
  tx(d,f,a,g.players[i].name,1540,y+16,19,white);
  tx(d,f,a,g.players[i].ready?"Ready":"Not Ready",1818,y+16,16,g.players[i].ready?rgb(0xc4fb66):rgb(0xff5479),Align::right);
 }
 if(g.player_count>4)tx(d,f,a,"+"+std::to_string(g.player_count-4)+" more players",1500,443,16,muted);
 panel(d,1428,482,415,267);
 tx(d,f,a,"Game Status",1454,531,26);
 d.line(1454,546,1818,546,1,rgb(0x647088,.4f));
 tx(d,f,a,"♧",1457,595,24,light);tx(d,f,a,std::to_string(g.player_count)+" Players",1515,594,20);
 tx(d,f,a,"○",1457,643,22,light);tx(d,f,a,g.mode?"Chill Mode":"Standard Mode",1515,642,20);
 tx(d,f,a,"▥",1457,691,22,light);tx(d,f,a,g.rounds?std::to_string(g.rounds)+" Challenges":"Endless",1515,690,20);
 tx(d,f,a,"~ 30-60 min",1515,733,18,muted);
 tx(d,f,a,"Good Drinks",1647,799,29,rgb(0xff4fb1));
 tx(d,f,a,"Good Friends",1626,835,29,rgb(0xff4fb1));
 tx(d,f,a,"Bad Decisions!",1605,873,29,rgb(0xff4fb1));
}
void footer(DrawList& d,const Font& f,unsigned a,const Game& g){
 d.gradient_rect({0,992,1920,88},0,rgb(0x050916,.96f),rgb(0x080b13));
 tx(d,f,a,"X  Confirm",72,1041,23);
 tx(d,f,a,"O  Back",257,1041,23);
 tx(d,f,a,g.screen==Screen::Players?"SQUARE  Rename   |   LEFT/RIGHT  Player count":g.screen==Screen::Game?"TRIANGLE  New session": "D-PAD  Navigate",1805,1041,21,light,Align::right);
}
void overlay(DrawList& d,const Font& f,unsigned a,Game& g){
 if(g.screen==Screen::Home){const char* labels[]={"Start Game","Players","Rules","Challenges","Settings"};
  const char* icons[]={">","+","=","*","o"};
  for(int i=0;i<5;i++)menu_button(d,f,a,i,labels[i],icons[i],g.focus==i);return;}
 if(g.screen==Screen::Game){
  panel(d,88,802,535,154,0x874be1,.96f);
  tx(d,f,a,"NOW PLAYING",119,845,18,rgb(0xfa89f9));
  tx(d,f,a,g.players[g.active].name+"'s turn",119,889,33);
  tx(d,f,a,g.spinning?"Bottle spinning...":"X  Spin the bottle!",119,928,23,rgb(0xffcd48));
  tx(d,f,a,"Round "+std::to_string(g.turn+1)+(g.rounds?" / "+std::to_string(g.rounds):""),596,934,19,light,Align::right);
  return;
 }
 if(g.screen==Screen::Result){
  panel(d,442,325,967,438,slices[g.winning_slice].color,.985f);
  tx(d,f,a,"THE BOTTLE HAS SPOKEN",925,382,22,rgb(slices[g.winning_slice].color),Align::center);
  tx(d,f,a,slices[g.winning_slice].label,925,462,52,white,Align::center);
  tx(d,f,a,g.players[g.active].name+", your challenge:",925,517,25,light,Align::center);
  std::string prompt=prompts[g.prompt_index].text;
  if(prompt.size()>48){size_t mid=prompt.rfind(' ',48);if(mid==std::string::npos)mid=48;
   tx(d,f,a,prompt.substr(0,mid),925,590,28,white,Align::center);
   tx(d,f,a,prompt.substr(mid+1),925,630,28,white,Align::center);
  }else tx(d,f,a,prompt,925,607,28,white,Align::center);
  tx(d,f,a,"X  Next player       O  Main menu",925,711,23,rgb(0xe49aff),Align::center);
  return;
 }
 panel(d,571,285,818,664,0xa739db,.99f);
 auto heading=[&](const char* text){tx(d,f,a,text,628,353,39,white);d.line(625,379,1333,379,2,rgb(0xf350d8,.65f));};
 if(g.screen==Screen::Players){
  heading("Players");
  tx(d,f,a,"Pass-and-play: one DualSense, 2-8 players",625,421,20,light);
  const uint32_t pc[]={0xff3c72,0x32b5ff,0x42ef9b,0xffaa36,0xd088ff,0xffe54f,0x40e9e6,0xff6e53};
  for(int i=0;i<g.player_count;i++){
   int col=i/4,row=i%4;
   const float x=622+col*359,y=443+row*104;
   d.bordered_rect({x,y,341,88},11,g.focus==i?rgb(0x50116a):rgb(0x101529),g.focus==i?3:1,rgb(g.focus==i?0xf758e8:0x55546d));
   d.circle(x+26,y+43,17,rgb(pc[i]));
   tx(d,f,a,"P"+std::to_string(i+1)+" "+g.players[i].name,x+53,y+36,23);
   tx(d,f,a,g.players[i].ready?"READY":"NOT READY",x+53,y+65,16,g.players[i].ready?rgb(0x62ff98):rgb(0xff667e));
  }
  tx(d,f,a,"X  Ready / Not Ready   |   SQUARE  Change name",620,896,19,rgb(0xffbbee));
  tx(d,f,a,"LEFT/RIGHT  Add or remove player   |   O  Back",620,923,17,light);
 }else if(g.screen==Screen::Rules){
  heading("Rules");
  const char* rules[]={"1. Set every player to Ready in Players.","2. Choose Start Game and press X to spin.","3. Follow the random Truth / Dare / Drink challenge.","4. Pass the controller to the next player.","5. Stop or skip any challenge at any time.","6. Non-alcoholic drinks are welcome. Drink responsibly."};
  for(int i=0;i<6;i++)tx(d,f,a,rules[i],627,453+67*i,23,i==5?rgb(0x62e4aa):light);
  tx(d,f,a,"O  Back",625,912,22,rgb(0xe6a4fd));
 }else if(g.screen==Screen::Challenges){
  heading("Challenges");
  tx(d,f,a,std::to_string(prompts.size())+" built-in challenges",625,426,20,light);
  int start=std::clamp(g.preview-4,0,int(prompts.size())-8);
  for(int i=start;i<start+8;i++){
   float y=443+(i-start)*52;
   if(i==g.preview)d.rounded_rect({626,y,703,47},8,rgb(0x5f2182));
   tx(d,f,a,std::to_string(i+1)+". "+std::string(prompts[i].text).substr(0,68),646,y+31,18);
  }
  tx(d,f,a,"UP/DOWN  Browse   |   O  Back",627,913,20,rgb(0xffb9ee));
 }else if(g.screen==Screen::Settings){
  heading("Settings");
  const char* labels[]={"Game mode", "Session length", "Reset to defaults"};
  const std::string vals[]={g.mode?"Chill":"Standard",g.rounds?std::to_string(g.rounds)+" rounds":"Endless","Apply"};
  for(int i=0;i<3;i++){
   float y=431+i*132;
   d.bordered_rect({619,y,724,112},12,i==g.settings_focus?rgb(0x50116a):rgb(0x0c1728),i==g.settings_focus?3:1,rgb(i==g.settings_focus?0xf758e8:0x444b67));
   tx(d,f,a,labels[i],652,y+47,27);
   tx(d,f,a,vals[i],1311,y+78,23,rgb(0x66dfff),Align::right);
  }
  tx(d,f,a,"X  Change   |   O  Back   |   Saves automatically",625,907,19,light);
 }else if(g.screen==Screen::Exit){
  heading("Exit Drink Roulette?");
  tx(d,f,a,"X  Quit to launcher",625,523,28);
  tx(d,f,a,"O  Keep playing",625,600,28);
 }
}
}
int main(){
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 hui::gfx::Renderer renderer;if(!renderer.init())hui::sys::park();
 Font font;
 std::string bytes=read("/app0/assets/fonts/inter-semibold.huifont");
 if(bytes.empty())bytes=read("/data/homebrew/PPSA99841/assets/fonts/inter-semibold.huifont");
 if(!font.load(bytes))hui::sys::park();
 unsigned atlas=renderer.batch().create_font_texture(font);
 Texture bg=background(renderer);
 hui::ps5::Pad pad;pad.open();
 Game g;load(g);g.seed(uint64_t(hui::sys::monotonic_us())^0xdecafbaacc11ULL);
 log("Boot started");
 uint32_t previous=0;uint64_t nav_next=0;
 auto last=uint64_t(hui::sys::monotonic_us());
 for(;;){
  const auto now=uint64_t(hui::sys::monotonic_us());
  float dt=std::clamp(float(now-last)/1000000.f,0.f,0.05f);last=now;
  std::array<hui::PadSample,64> samples{};size_t n=pad.read(samples);
  uint32_t pressed=0,held=previous;bool connected=false;
  for(size_t i=0;i<n;i++){
   if(!samples[i].connected)continue;
   connected=true;uint32_t buttons=samples[i].buttons;
   if(samples[i].left_y<65)buttons|=hui::pad_bits::kUp;
   if(samples[i].left_y>190)buttons|=hui::pad_bits::kDown;
   if(samples[i].left_x<65)buttons|=hui::pad_bits::kLeft;
   if(samples[i].left_x>190)buttons|=hui::pad_bits::kRight;
   pressed|=buttons&~previous;previous=buttons;held=buttons;
  }
  if(!connected){held=0;previous=0;}
  const uint32_t directions=held&(hui::pad_bits::kUp|hui::pad_bits::kDown|hui::pad_bits::kLeft|hui::pad_bits::kRight);
  uint32_t nav=pressed&directions;
  if(nav)nav_next=now+290000;
  else if(directions&&now>=nav_next){nav=directions;nav_next=now+125000;}
  else if(!directions)nav_next=0;
  auto was=[&](uint32_t v){return (pressed&v)!=0;};
  const bool up=nav&hui::pad_bits::kUp,down=nav&hui::pad_bits::kDown;
  const bool left=nav&hui::pad_bits::kLeft,right=nav&hui::pad_bits::kRight;
  bool save_changed=false;
  if(g.screen==Screen::Home){
   if(up)g.focus=(g.focus+4)%5;
   if(down)g.focus=(g.focus+1)%5;
   if(was(hui::pad_bits::kCross)){
    switch(g.focus){case 0:g.start_session();break;case 1:g.screen=Screen::Players;g.focus=0;break;
     case 2:g.screen=Screen::Rules;break;case 3:g.screen=Screen::Challenges;g.preview=0;break;
     default:g.screen=Screen::Settings;g.settings_focus=0;break;}
   }
   if(was(hui::pad_bits::kCircle))g.screen=Screen::Exit;
  }else if(g.screen==Screen::Players){
   if(up)g.focus=(g.focus+g.player_count-1)%g.player_count;
   if(down)g.focus=(g.focus+1)%g.player_count;
   if(left){g.player_count=std::max(2,g.player_count-1);g.focus=std::min(g.focus,g.player_count-1);save_changed=true;}
   if(right){g.player_count=std::min(8,g.player_count+1);save_changed=true;}
   if(was(hui::pad_bits::kCross))g.players[g.focus].ready=!g.players[g.focus].ready;
   if(was(hui::pad_bits::kSquare)){g.cycle_name(g.focus);save_changed=true;}
   if(was(hui::pad_bits::kCircle)){g.screen=Screen::Home;g.focus=0;}
  }else if(g.screen==Screen::Rules||g.screen==Screen::Challenges||g.screen==Screen::Settings){
   if(g.screen==Screen::Challenges){if(up)g.preview=std::max(0,g.preview-1);if(down)g.preview=std::min(int(prompts.size())-1,g.preview+1);}
   if(g.screen==Screen::Settings){
    if(up)g.settings_focus=(g.settings_focus+2)%3;
    if(down)g.settings_focus=(g.settings_focus+1)%3;
    if(was(hui::pad_bits::kCross)||left||right){
     if(g.settings_focus==0)g.mode=1-g.mode;
     if(g.settings_focus==1)g.rounds=g.rounds==15?30:g.rounds==30?0:15;
     if(g.settings_focus==2){g.mode=0;g.rounds=15;}
     save_changed=true;
    }
   }
   if(was(hui::pad_bits::kCircle)){bool settings=(g.screen==Screen::Settings);g.screen=Screen::Home;g.focus=settings?4:0;}
  }else if(g.screen==Screen::Game){
   if(was(hui::pad_bits::kCross))g.start_spin();
   if(was(hui::pad_bits::kTriangle)&&!g.spinning)g.start_session();
   if(was(hui::pad_bits::kCircle)&&!g.spinning){g.screen=Screen::Home;g.focus=0;}
  }else if(g.screen==Screen::Result){
   if(was(hui::pad_bits::kCross))g.next_turn();
   if(was(hui::pad_bits::kCircle)){g.screen=Screen::Home;g.focus=0;}
  }else if(g.screen==Screen::Exit){
   if(was(hui::pad_bits::kCross)){save(g);pad.close();if(bg.id)glDeleteTextures(1,&bg.id);renderer.release();display.close();hui::sys::quit();}
   if(was(hui::pad_bits::kCircle))g.screen=Screen::Home;
  }
  if(was(hui::pad_bits::kOptions)&&g.screen==Screen::Home){g.screen=Screen::Settings;g.settings_focus=0;}
  if(save_changed)save(g);
  if(g.update(dt)){pad.rumble(.6f,.22f);log("Spin complete");}
  pad.tick(dt);
  DrawList d;
  if(bg.id){d.image(bg.id,{0,0,1920,1080},kFullUv,white);if(!bg.reference)title(d,font,atlas);}else{party_backdrop(d);title(d,font,atlas);}
  // Dynamic UI panels replace the baked-in reference panels when photo is present.
  if(bg.id){d.rounded_rect({71,293,532,620},14,rgb(0x080b16,.91f));d.rounded_rect({1415,107,447,665},15,rgb(0x090b17,.88f));}
  wheel(d,font,atlas,g);
  cards(d,font,atlas,g);
  overlay(d,font,atlas,g);
  tx(d,font,atlas,"by YAHBALLON",1747,963,17,light,Align::right);
  tx(d,font,atlas,"v0.1.1",1851,963,18,light,Align::right);
  footer(d,font,atlas,g);
  renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
