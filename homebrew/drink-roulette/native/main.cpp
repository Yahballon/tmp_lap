// SPDX-License-Identifier: GPL-3.0-or-later
// Drink Roulette by YAHBALLON v0.1.6 — native PS5 / ps5-homebrew-ui.
#include "game.hpp"
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "core/input.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
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
 if(fread(content.data(),1,size_t(n),f)!=size_t(n))content.clear();
 fclose(f);return content;
}
void log(const char* text){mkdir("/data/DrinkRoulette",0755);if(FILE* f=fopen("/data/DrinkRoulette/drink-roulette.log","ab")){fprintf(f,"[DrinkRoulette v0.1.6] %s\n",text);fclose(f);}}
// Persist the setup atomically. Only controller-keyboard characters are saved.
void save(const Game& g){
 mkdir("/data/DrinkRoulette",0755);
 FILE* f=fopen("/data/DrinkRoulette/settings.tmp","wb");if(!f)return;
 fprintf(f,"v=2\nsetup=%d\nplayers=%d\nmode=%d\npack=%d\nrounds=%d\n",g.setup_complete?1:0,g.player_count,g.mode,g.pack,g.rounds);
 for(int i=0;i<8;i++)fprintf(f,"name%d=%s\n",i,g.players[i].name.c_str());
 if(fclose(f)==0)rename("/data/DrinkRoulette/settings.tmp","/data/DrinkRoulette/settings.ini");
}
std::string safe_name(std::string text){
 std::string s;for(char ch:text){
  if(s.size()>=16)break;
  if((ch>='A'&&ch<='Z')||(ch>='a'&&ch<='z')||(ch>='0'&&ch<='9')||ch==' '||ch=='-'||ch=='_'||ch=='.')s.push_back(ch);
 }
 return s;
}
void load(Game& g){
 FILE* f=fopen("/data/DrinkRoulette/settings.ini","rb");if(!f)return;
 char line[256]={};int version=0;bool setup=false;
 while(fgets(line,sizeof(line),f)){
  int n=0;
  if(sscanf(line,"v=%d",&n)==1){version=n;continue;}
  if(version<1||version>2)continue;
  if(sscanf(line,"setup=%d",&n)==1)setup=n!=0;
  else if(sscanf(line,"players=%d",&n)==1)g.player_count=std::clamp(n,2,8);
  else if(sscanf(line,"mode=%d",&n)==1)g.mode=std::clamp(n,0,4);
  else if(sscanf(line,"pack=%d",&n)==1)g.pack=std::clamp(n,0,2);
  else if(sscanf(line,"rounds=%d",&n)==1&&n>=0&&n<=120)g.rounds=n;
  else{
   int i=-1;char name[100]={};
   if(sscanf(line,"name%d=%99[^\n]",&i,name)==2&&i>=0&&i<8){
    const auto clean=safe_name(name);if(!clean.empty())g.players[i].name=clean;
   }
  }
 }
 fclose(f);
 // v1 players get a fresh setup prompt once, even after upgrading.
 g.setup_complete=version==2&&setup;
 g.screen=g.setup_complete?Screen::Home:Screen::SetupCount;
}
constexpr const char* mode_names[]={"Standard","Wild","Couples","Quickfire","Truth & Dare"};
constexpr const char* pack_names[]={"Light","Normal","All Challenges"};
constexpr const char* keyboard_keys[]={
 "A","B","C","D","E","F","G","H","I","J",
 "K","L","M","N","O","P","Q","R","S","T",
 "U","V","W","X","Y","Z","0","1","2","3",
 "4","5","6","7","8","9","SPACE","DEL","OK","CANCEL"
};
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
Texture load_bottle(Renderer& r){
 Texture result;
 // Raw RGBA8 is produced and ZIP-verified in CI from the source bottle.png.
 // Loading it directly avoids device-specific stb PNG decode failures.
 constexpr int side=512;
 constexpr size_t rawBytes=size_t(side)*side*4;
 constexpr const char* rawPaths[]={
   "/app0/assets/bottle.rgba",
   "/data/homebrew/PPSA99841/assets/bottle.rgba",
   "assets/bottle.rgba",
   "/mnt/usb0/homebrew/PPSA99841/assets/bottle.rgba"
 };
 char message[256];
 for(const char* path:rawPaths){
  auto pixels=read(path,rawBytes+1);
  if(pixels.empty())continue;
  if(pixels.size()!=rawBytes){
   snprintf(message,sizeof(message),"Bottle raw bad size %s: %u",path,unsigned(pixels.size()));
   log(message);continue;
  }
  result.id=r.batch().create_texture(side,side,reinterpret_cast<const uint8_t*>(pixels.data()));
  result.width=side;result.height=side;
  if(result.id){
   snprintf(message,sizeof(message),"Bottle loaded raw RGBA: %s (%dx%d, id=%u)",path,side,side,unsigned(result.id));
   log(message);return result;
  }
  snprintf(message,sizeof(message),"Bottle raw GPU texture creation failed: %s",path);
  log(message);
 }
 log("Bottle raw not available: trying source PNG");
 constexpr const char* pngPaths[]={
  "/app0/assets/bottle.png",
  "/data/homebrew/PPSA99841/assets/bottle.png",
  "assets/bottle.png",
  "/mnt/usb0/homebrew/PPSA99841/assets/bottle.png"
 };
 for(const char* path:pngPaths){
  auto bytes=read(path);
  if(bytes.empty())continue;
  int w=0,h=0,n=0;
  auto* rgba=stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(bytes.data()),
                                  int(bytes.size()),&w,&h,&n,4);
  if(!rgba){
   snprintf(message,sizeof(message),"Bottle PNG decode failure %s: %.96s",path,
            stbi_failure_reason()?stbi_failure_reason():"unknown");
   log(message);continue;
  }
  if(w<1||h<1||w>2048||h>2048){
   snprintf(message,sizeof(message),"Bottle invalid PNG size: %d x %d",w,h);
   log(message);stbi_image_free(rgba);continue;
  }
  result.id=r.batch().create_texture(w,h,rgba);
  result.width=w;result.height=h;
  stbi_image_free(rgba);
  if(result.id){
   snprintf(message,sizeof(message),"Bottle loaded PNG: %s (%dx%d, id=%u)",path,w,h,unsigned(result.id));
   log(message);return result;
  }
  log("Bottle PNG GPU texture creation failed");
 }
 log("Bottle FAILED: all native RGBA and PNG paths unavailable");
 return {};
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
void wheel(DrawList& d,const Font& f,unsigned a,const Game& g,const Texture& bottle,bool photoWheel){
 const float cx=photoWheel?962.f:996.f,cy=photoWheel?607.f:631.f,r=349.f;
 // The reference JPEG already contains the colorful roulette wheel.
 // Repainting it shifted wheel/labels 34x24 pixels and doubled the whole UI.
 if(!photoWheel){
  d.shadow({cx-r-17,cy-r+26,2*(r+17),2*(r+17)},350,32,rgb(0x000000,.82f));
  d.circle(cx,cy+10,r+25,rgb(0x090707));
  d.ring(cx,cy+9,r+23,10,rgb(0xa66b33));
  d.circle(cx,cy,r+10,rgb(0x060815));
  const float step=tau/10.f;
  for(int i=0;i<10;i++){
   float begin=(i-.5f)*step;
   float sweep=step-.015f;
   d.arc(cx,cy,r,175,begin,sweep,rgb(slices[i].color),false);
   d.arc(cx,cy,r+2,5,begin+.017f,sweep-.032f,rgb(0xffffff,.8f),false);
   float angle=i*step,x=cx+std::sin(angle)*270,y=cy-std::cos(angle)*270;
   std::string label=slices[i].label;
   if(label=="GROUP DRINK"){
    tx(d,f,a,"GROUP",x,y-8,17,white,Align::center);
    tx(d,f,a,"DRINK",x,y+15,17,white,Align::center);
   }else tx(d,f,a,label,x,y,19,white,Align::center);
  }
  d.ring(cx,cy,r+9,5,rgb(0xe4b96e));
 }
 d.circle(cx,cy,179,rgb(0x1c1418));
 d.ring(cx,cy,178,4,rgb(0x795534));
 d.circle(cx,cy,162,rgb(0x100d12));
 // Radial woodgrain/depth in the fixed inner disc.
 for(int i=0;i<10;i++){float a=i*tau/10;d.line(cx+std::sin(a)*22,cy-std::cos(a)*22,cx+std::sin(a)*160,cy-std::cos(a)*160,2,rgb(0xb17d47,.12f));}
 // The photograph is a square transparent atlas with its gold pivot centered.
 // The backend rotates image UVs: keep an equally square draw quad.
 constexpr float bottleAtlasHalf=205.f;
 if(bottle.id){
  d.rotated_image(bottle.id,
    {cx-bottleAtlasHalf,cy-bottleAtlasHalf,2.f*bottleAtlasHalf,2.f*bottleAtlasHalf},
    pi*.5f-g.angle,white);
 }else{
  // Visual safety net when installation/decoding fails. No error text in
  // the center of the game wheel; details go into the PS5 log instead.
  const float rotation=pi*.5f-g.angle;
  const float cc=std::cos(rotation),ss=std::sin(rotation);
  auto stroke=[&](float x1,float y1,float x2,float y2,float thickness,Color tint){
   d.line(cx+x1*cc-y1*ss,cy+x1*ss+y1*cc,
          cx+x2*cc-y2*ss,cy+x2*ss+y2*cc,thickness,tint);
  };
  stroke(-125,0,68,0,65,rgb(0x0c542e));
  stroke(-128,0,72,0,52,rgb(0x05834c));
  stroke(62,0,131,0,28,rgb(0x066236));
  stroke(118,0,141,0,26,rgb(0xe8ab56));
  stroke(-112,-20,52,-20,6,rgb(0xa6f8bb));
  stroke(-112,20,45,20,4,rgb(0x003c21));
  stroke(75,-6,121,-6,3,rgb(0xbafac5));
 }
 d.circle(cx,cy,15,rgb(0xd8a74e));
 d.ring(cx,cy,13,3,rgb(0xffe3a0));d.circle(cx,cy,5,rgb(0xfff8db));
 // Highlight currently selected sector with a small fixed needle at the bottle tip.
 if(g.spinning){d.ring(cx,cy,r+14,3,rgb(0xffffff,.25f));}
}
void cards(DrawList& d,const Font& f,unsigned a,const Game& g){
 panel(d,1373,116,470,344);
 tx(d,f,a,"Players ("+std::to_string(g.player_count)+")",1399,163,25);
 d.line(1399,180,1818,180,1,rgb(0x647088,.4f));
 const uint32_t pc[]={0xff3c72,0x32b5ff,0x42ef9b,0xffaa36,0xd088ff,0xffe54f,0x40e9e6,0xff6e53};
 for(int i=0;i<g.player_count&&i<4;i++){
  float y=209+i*56;
  d.circle(1411,y+10,20,rgb(pc[i]));
  tx(d,f,a,"P"+std::to_string(i+1),1441,y+16,18,rgb(pc[i]));
  tx(d,f,a,g.players[i].name,1485,y+16,19,white);
  tx(d,f,a,g.players[i].ready?"Ready":"Not Ready",1818,y+16,16,g.players[i].ready?rgb(0xc4fb66):rgb(0xff5479),Align::right);
 }
 if(g.player_count>4)tx(d,f,a,"+"+std::to_string(g.player_count-4)+" more players",1445,443,16,muted);
 panel(d,1373,482,470,267);
 tx(d,f,a,"Game Status",1399,531,26);
 d.line(1399,546,1818,546,1,rgb(0x647088,.4f));
 tx(d,f,a,"♧",1457,595,24,light);tx(d,f,a,std::to_string(g.player_count)+" Players",1515,594,20);
 tx(d,f,a,"○",1457,643,22,light);tx(d,f,a,std::string(mode_names[g.mode])+" Mode",1515,642,20);
 tx(d,f,a,"▥",1457,691,22,light);tx(d,f,a,std::to_string(g.available_prompt_count())+" Challenges",1515,690,20);
 tx(d,f,a,"~ 30-60 min",1515,733,18,muted);

}
void footer(DrawList& d,const Font& f,unsigned a,const Game& g){
 d.rounded_rect({0,964,1920,116},0,rgb(0x050916,1.f));
 tx(d,f,a,"X  Confirm",72,1041,23);
 tx(d,f,a,"O  Back",257,1041,23);
 tx(d,f,a,g.screen==Screen::Players?"SQUARE  Enter name   |   LEFT/RIGHT  Player count":g.screen==Screen::Home?"TRIANGLE  Rules   |   D-PAD  Navigate":g.screen==Screen::Game?"TRIANGLE  New session": "D-PAD  Navigate",1805,1041,21,light,Align::right);
}
void wrapped(DrawList& d,const Font& f,unsigned a,const std::string& message,float cx,float y,int fontSize,int maxChars,int maxLines){
 std::vector<std::string> lines;
 std::string current,word;
 auto flush=[&](){if(!current.empty()){lines.push_back(current);current.clear();}};
 for(size_t i=0;i<=message.size();i++){
  char ch=i<message.size()?message[i]:' ';
  if(ch==' '||ch=='\n'){
   if(!word.empty()){
    if(current.size()+word.size()+1>size_t(maxChars))flush();
    if(!current.empty())current+=' ';
    current+=word;word.clear();
   }
   if(ch=='\n')flush();
  }else word+=ch;
 }
 flush();
 const int count=std::min(int(lines.size()),maxLines);
 for(int i=0;i<count;i++)tx(d,f,a,lines[i],cx,y+float(i)*41,fontSize,white,Align::center);
}
void overlay(DrawList& d,const Font& f,unsigned a,Game& g){
 if(g.screen==Screen::Home){
  const uint32_t pc[]={0xff3c72,0x32b5ff,0x42ef9b,0xffaa36,0xd088ff,0xffe54f,0x40e9e6,0xff6e53};
  auto item=[&](int index,const std::string& name,const char* symbol,float y){
   const bool focus=index==g.focus;
   if(focus)d.glow({94,y,475,72},13,15,rgb(0xeb22fc,.75f));
   d.bordered_rect({96,y,475,69},14,focus?rgb(0x71008c,.96f):rgb(0x0b1426,.98f),focus?3.f:1.5f,rgb(focus?0xf741fc:0x334664));
   tx(d,f,a,symbol,119,y+47,30,focus?rgb(0xffd65c):light);
   tx(d,f,a,name,202,y+44,28,white);
   tx(d,f,a,">",540,y+45,27,light,Align::right);
  };
  item(0,"Start Game",">",296);
  item(1,"Players ("+std::to_string(g.player_count)+")","+",381);
  const int visible=std::min(g.player_count,4);
  for(int i=0;i<visible;i++){
   float y=453+i*56;
   d.bordered_rect({105,y,454,51},11,rgb(0x0a1729,.96f),1,rgb(0x38506a));
   d.circle(136,y+25,16,rgb(pc[i]));
   tx(d,f,a,"P"+std::to_string(i+1),161,y+32,20,rgb(pc[i]));
   tx(d,f,a,g.players[i].name.substr(0,16),226,y+32,24);
   tx(d,f,a,"edit",526,y+32,18,muted,Align::right);
  }
  if(g.player_count>4)tx(d,f,a,"+"+std::to_string(g.player_count-4)+" more - open Players",118,695,16,light);
  item(2,"Game Mode","*",731);
  item(3,"Challenges","#",810);
  item(4,"Settings","o",889);
  return;
 }
 if(g.screen==Screen::Game){
  // Position this live status card immediately below the neon title.
  // Keep it within the opaque left sidebar and clear of the wheel.
  panel(d,88,309,490,154,0x874be1,.96f);
  tx(d,f,a,"NOW PLAYING",119,354,18,rgb(0xfa89f9));
  tx(d,f,a,g.players[g.active].name+"'s turn",119,399,33);
  tx(d,f,a,g.spinning?"Bottle spinning...":"X  Spin the bottle!",119,439,23,rgb(0xffcd48));
  tx(d,f,a,"Round "+std::to_string(g.turn+1)+(g.rounds?" / "+std::to_string(g.rounds):""),551,445,19,light,Align::right);
  return;
 }
 if(g.screen==Screen::Result){
  panel(d,442,325,967,438,slices[g.winning_slice].color,.985f);
  tx(d,f,a,"THE BOTTLE HAS SPOKEN",925,382,22,rgb(slices[g.winning_slice].color),Align::center);
  tx(d,f,a,slices[g.winning_slice].label,925,462,52,white,Align::center);
  tx(d,f,a,g.players[g.active].name+", your challenge:",925,517,25,light,Align::center);
  wrapped(d,f,a,prompts[g.prompt_index].text,925,573,27,50,4);
  tx(d,f,a,"X  Next player       O  Main menu",925,711,23,rgb(0xe49aff),Align::center);
  return;
 }
 panel(d,571,285,818,664,0xa739db,.99f);
 auto heading=[&](const std::string& text){tx(d,f,a,text,628,353,39,white);d.line(625,379,1333,379,2,rgb(0xf350d8,.65f));};
 if(g.screen==Screen::SetupCount){
  heading("WELCOME - HOW MANY PLAYERS?");
  tx(d,f,a,"Choose how many friends are joining tonight",625,437,24,light);
  tx(d,f,a,"<",758,665,65,rgb(0xffbd38));
  tx(d,f,a,std::to_string(g.player_count),962,683,114,rgb(0xffe16f),Align::center);
  tx(d,f,a,">",1144,665,65,rgb(0xffbd38));
  tx(d,f,a,"2 - 8 players    |    LEFT/RIGHT Choose",962,798,25,white,Align::center);
  tx(d,f,a,"X  Continue",962,917,24,rgb(0xffb9ee),Align::center);
 }else if(g.screen==Screen::SetupNames){
  heading("WHO IS PLAYING?");
  tx(d,f,a,"Select a player and press X to enter their name",625,416,22,light);
  const uint32_t pc[]={0xff3c72,0x32b5ff,0x42ef9b,0xffaa36,0xd088ff,0xffe54f,0x40e9e6,0xff6e53};
  for(int i=0;i<g.player_count;i++){
   const float x=617+(i/4)*368.f,y=447+(i%4)*90.f;
   bool selected=g.setup_focus==i;
   d.bordered_rect({x,y,355,80},12,selected?rgb(0x521476):rgb(0x0b162b),selected?2.5f:1.2f,rgb(selected?0xed50ff:0x40526b));
   d.circle(x+26,y+38,17,rgb(pc[i]));
   tx(d,f,a,"P"+std::to_string(i+1),x+53,y+47,20,rgb(pc[i]));
   tx(d,f,a,g.players[i].name,x+105,y+47,22);
  }
  tx(d,f,a,"X  Edit name    |    SQUARE  Continue",958,909,23,rgb(0xffb9ee),Align::center);
 }else if(g.screen==Screen::EditName){
  heading("ENTER PLAYER "+std::to_string(g.editing_player+1)+" NAME");
  d.bordered_rect({622,397,711,81},12,rgb(0x1e1939),2,rgb(0xfc4dee));
  tx(d,f,a,g.editing_name.empty()?"Type a name ...":g.editing_name,650,450,31,white);
  for(int i=0;i<40;i++){
   const float x=627+(i%10)*70.f,y=510+(i/10)*81.f;
   bool focus=g.keyboard_cursor==i;
   d.bordered_rect({x,y,64,70},9,rgb(focus?0x6f139a:0x152139),focus?2.5f:1.f,rgb(focus?0xfa45ff:0x41526d));
   tx(d,f,a,keyboard_keys[i],x+32,y+43,i>=36?12:23,white,Align::center);
  }
  tx(d,f,a,"D-PAD  Select    X  Type    TRIANGLE  Delete    OPTIONS  Save",958,906,19,light,Align::center);
 }else if(g.screen==Screen::SetupMode){
  heading("CHOOSE GAME MODE");
  for(int i=0;i<5;i++){
   float y=418+i*90;
   d.bordered_rect({620,y,713,75},12,rgb(g.mode==i?0x551474:0x10192d),g.mode==i?3:1,rgb(g.mode==i?0xf04aff:0x354663));
   tx(d,f,a,mode_names[i],665,y+49,30,white);
  }
  tx(d,f,a,"UP/DOWN  Select    X  Next    O  Back",945,917,20,light,Align::center);
 }else if(g.screen==Screen::SetupPack){
  heading("CHOOSE CHALLENGE PACK");
  for(int i=0;i<3;i++){
   const float y=470+i*122;
   d.bordered_rect({621,y,713,96},12,rgb(g.pack==i?0x551474:0x10192d),g.pack==i?3:1,rgb(g.pack==i?0xf04aff:0x354663));
   tx(d,f,a,pack_names[i],666,y+45,29);
   int count=0;for(const auto& p:prompts)if(p.level<=i&&g.category_allowed(p.category))++count;
   tx(d,f,a,std::to_string(count)+" challenges",1298,y+61,21,rgb(0x7ce7ff),Align::right);
  }
  tx(d,f,a,"UP/DOWN  Select    X  Next    O  Back",945,917,20,light,Align::center);
 }else if(g.screen==Screen::SetupSummary){
  heading("READY TO PLAY?");
  tx(d,f,a,std::to_string(g.player_count)+" Players",661,474,35);
  tx(d,f,a,std::string(mode_names[g.mode])+" Mode",661,537,35);
  tx(d,f,a,std::to_string(g.available_prompt_count())+" Challenges",661,600,35);
  tx(d,f,a,std::string("Challenge pack: ")+pack_names[g.pack],661,663,27,light);
  tx(d,f,a,"Every challenge is optional - pass any time.",662,760,23,rgb(0x6ef2c1));
  tx(d,f,a,"X  Enter Game Lobby    O  Back",960,921,24,rgb(0xffb9ee),Align::center);
 }else if(g.screen==Screen::Players){
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
  tx(d,f,a,"X  Ready / Not Ready   |   SQUARE  Enter name",620,896,19,rgb(0xffbbee));
  tx(d,f,a,"LEFT/RIGHT  Add or remove player   |   O  Back",620,923,17,light);
 }else if(g.screen==Screen::Rules){
  heading("Rules");
  const char* rules[]={"1. Set every player to Ready in Players.","2. Choose Start Game and press X to spin.","3. Follow the random Truth / Dare / Drink challenge.","4. Pass the controller to the next player.","5. Stop or skip any challenge at any time.","6. Non-alcoholic drinks are welcome. Drink responsibly."};
  for(int i=0;i<6;i++)tx(d,f,a,rules[i],627,453+67*i,23,i==5?rgb(0x62e4aa):light);
  tx(d,f,a,"O  Back",625,912,22,rgb(0xe6a4fd));
 }else if(g.screen==Screen::Challenges){
  heading("Challenges");
  tx(d,f,a,std::to_string(g.available_prompt_count())+" in this mode / "+std::to_string(prompts.size())+" total challenges",625,426,20,light);
  int start=std::clamp(g.preview-4,0,int(prompts.size())-8);
  for(int i=start;i<start+8;i++){
   float y=443+(i-start)*52;
   if(i==g.preview)d.rounded_rect({626,y,703,47},8,rgb(0x5f2182));
   tx(d,f,a,std::to_string(i+1)+". "+std::string(prompts[i].text).substr(0,68),646,y+31,18);
  }
  tx(d,f,a,"UP/DOWN  Browse   |   O  Back",627,913,20,rgb(0xffb9ee));
 }else if(g.screen==Screen::Settings){
  heading("Settings");
  const char* labels[]={"Game mode", "Challenge pack", "Session length", "Run player setup again"};
  const std::string vals[]={mode_names[g.mode],pack_names[g.pack],g.rounds?std::to_string(g.rounds)+" turns":"Endless","Launch"};
  for(int i=0;i<4;i++){
   float y=419+i*110;
   d.bordered_rect({619,y,724,95},12,i==g.settings_focus?rgb(0x50116a):rgb(0x0c1728),i==g.settings_focus?3:1,rgb(i==g.settings_focus?0xf758e8:0x444b67));
   tx(d,f,a,labels[i],652,y+47,27);
   tx(d,f,a,vals[i],1311,y+78,23,rgb(0x66dfff),Align::right);
  }
  tx(d,f,a,"X / LEFT-RIGHT  Change   |   O  Back",625,918,20,light);
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
 Texture bottle=load_bottle(renderer);
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
  if(g.screen==Screen::SetupCount){
   if(left){g.player_count=std::max(2,g.player_count-1);save_changed=true;}
   if(right){g.player_count=std::min(8,g.player_count+1);save_changed=true;}
   if(was(hui::pad_bits::kCross)){g.screen=Screen::SetupNames;g.setup_focus=0;}
  }else if(g.screen==Screen::SetupNames){
   if(up)g.setup_focus=(g.setup_focus+g.player_count-1)%g.player_count;
   if(down)g.setup_focus=(g.setup_focus+1)%g.player_count;
   if(left&&g.setup_focus>=4)g.setup_focus-=4;
   if(right&&g.setup_focus+4<g.player_count)g.setup_focus+=4;
   if(was(hui::pad_bits::kCross))g.begin_name_edit(g.setup_focus,Screen::SetupNames);
   if(was(hui::pad_bits::kSquare))g.screen=Screen::SetupMode;
   if(was(hui::pad_bits::kCircle))g.screen=Screen::SetupCount;
  }else if(g.screen==Screen::EditName){
   if(left)g.keyboard_cursor=(g.keyboard_cursor/10)*10+(g.keyboard_cursor+9)%10;
   if(right)g.keyboard_cursor=(g.keyboard_cursor/10)*10+(g.keyboard_cursor+1)%10;
   if(up)g.keyboard_cursor=(g.keyboard_cursor+30)%40;
   if(down)g.keyboard_cursor=(g.keyboard_cursor+10)%40;
   if(was(hui::pad_bits::kCross)){
    int index=g.keyboard_cursor;
    if(index<36){if(g.editing_name.size()<16)g.editing_name+=keyboard_keys[index];}
    else if(index==36){if(g.editing_name.size()<16)g.editing_name+=' ';}
    else if(index==37){if(!g.editing_name.empty())g.editing_name.pop_back();}
    else if(index==38){g.apply_name_edit();save_changed=true;}
    else g.screen=g.keyboard_return;
   }
   if(was(hui::pad_bits::kTriangle)&&!g.editing_name.empty())g.editing_name.pop_back();
   if(was(hui::pad_bits::kOptions)){g.apply_name_edit();save_changed=true;}
   if(was(hui::pad_bits::kCircle))g.screen=g.keyboard_return;
  }else if(g.screen==Screen::SetupMode){
   if(up||left)g.mode=(g.mode+4)%5;
   if(down||right)g.mode=(g.mode+1)%5;
   if(was(hui::pad_bits::kCross))g.screen=Screen::SetupPack;
   if(was(hui::pad_bits::kCircle))g.screen=Screen::SetupNames;
  }else if(g.screen==Screen::SetupPack){
   if(up||left)g.pack=(g.pack+2)%3;
   if(down||right)g.pack=(g.pack+1)%3;
   if(was(hui::pad_bits::kCross))g.screen=Screen::SetupSummary;
   if(was(hui::pad_bits::kCircle))g.screen=Screen::SetupMode;
  }else if(g.screen==Screen::SetupSummary){
   if(was(hui::pad_bits::kCross)){
    g.setup_complete=true;g.screen=Screen::Home;g.focus=0;
    for(int i=0;i<g.player_count;i++)g.players[i].ready=true;
    save_changed=true;
   }
   if(was(hui::pad_bits::kCircle))g.screen=Screen::SetupPack;
  }else if(g.screen==Screen::Home){
   if(up)g.focus=(g.focus+4)%5;
   if(down)g.focus=(g.focus+1)%5;
   if(was(hui::pad_bits::kCross)){
    switch(g.focus){case 0:g.start_session();break;case 1:g.screen=Screen::Players;g.focus=0;break;
     case 2:g.screen=Screen::Settings;g.settings_focus=0;break;case 3:g.screen=Screen::Challenges;g.preview=0;break;
     default:g.screen=Screen::Settings;g.settings_focus=0;break;}
   }
   if(was(hui::pad_bits::kTriangle))g.screen=Screen::Rules;
   if(was(hui::pad_bits::kCircle))g.screen=Screen::Exit;
  }else if(g.screen==Screen::Players){
   if(up)g.focus=(g.focus+g.player_count-1)%g.player_count;
   if(down)g.focus=(g.focus+1)%g.player_count;
   if(left){g.player_count=std::max(2,g.player_count-1);g.focus=std::min(g.focus,g.player_count-1);save_changed=true;}
   if(right){g.player_count=std::min(8,g.player_count+1);save_changed=true;}
   if(was(hui::pad_bits::kCross)){g.players[g.focus].ready=!g.players[g.focus].ready;save_changed=true;}
   if(was(hui::pad_bits::kSquare))g.begin_name_edit(g.focus,Screen::Players);
   if(was(hui::pad_bits::kCircle)){g.screen=Screen::Home;g.focus=1;}
  }else if(g.screen==Screen::Rules||g.screen==Screen::Challenges||g.screen==Screen::Settings){
   if(g.screen==Screen::Challenges){if(up)g.preview=std::max(0,g.preview-1);if(down)g.preview=std::min(int(prompts.size())-1,g.preview+1);}
   if(g.screen==Screen::Settings){
    if(up)g.settings_focus=(g.settings_focus+3)%4;
    if(down)g.settings_focus=(g.settings_focus+1)%4;
    if(was(hui::pad_bits::kCross)||left||right){
     if(g.settings_focus==0)g.mode=(g.mode+1)%5;
     else if(g.settings_focus==1)g.pack=(g.pack+1)%3;
     else if(g.settings_focus==2)g.rounds=g.rounds==15?30:g.rounds==30?60:g.rounds==60?0:15;
     else{g.start_new_setup();}
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
   if(was(hui::pad_bits::kCross)){save(g);pad.close();if(bg.id)glDeleteTextures(1,&bg.id);if(bottle.id)glDeleteTextures(1,&bottle.id);renderer.release();display.close();hui::sys::quit();}
   if(was(hui::pad_bits::kCircle))g.screen=Screen::Home;
  }
  if(was(hui::pad_bits::kOptions)&&g.screen==Screen::Home){g.screen=Screen::Settings;g.settings_focus=0;}
  if(save_changed)save(g);
  if(g.update(dt)){pad.rumble(.6f,.22f);log("Spin complete");}
  pad.tick(dt);
  DrawList d;
  if(bg.id){d.image(bg.id,{0,0,1920,1080},kFullUv,white);if(!bg.reference)title(d,font,atlas);}else{party_backdrop(d);title(d,font,atlas);}
  // Dynamic UI panels replace the baked-in reference panels when photo is present.
  // The artwork has static sidebars baked in; mask them completely.
  if(bg.reference){
   d.rounded_rect({65,277,555,713},15,rgb(0x080d19,1.f));
   d.rounded_rect({1340,92,542,697},16,rgb(0x080d19,1.f));
   d.rounded_rect({0,964,1920,116},0,rgb(0x050916,1.f));
  }
  wheel(d,font,atlas,g,bottle,bg.reference);
  cards(d,font,atlas,g);
  overlay(d,font,atlas,g);
  footer(d,font,atlas,g);
  renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
