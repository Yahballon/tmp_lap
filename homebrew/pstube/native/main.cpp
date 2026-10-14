// SPDX-License-Identifier: GPL-3.0-or-later
// PSTUBE alpha: native PS5 search/library UI. Media playback not implemented.
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "pstube.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <string>
#include <vector>
using namespace hui::gfx;
namespace {
const Color white=Color::rgb(0xf6f9ff),dim=Color::rgb(0x91a4c7),
 blue=Color::rgb(0x008cff),panel=Color::rgb(0x0a1d36),edge=Color::rgb(0x234b77),
 green=Color::rgb(0x0cc6a2);
std::string load(const char* path){
 FILE* f=fopen(path,"rb");if(!f)return {};if(fseek(f,0,SEEK_END)){fclose(f);return {};}
 long n=ftell(f);if(n<1||n>10000000){fclose(f);return {};}rewind(f);
 std::string out((size_t)n,0);size_t read=fread(out.data(),1,out.size(),f);fclose(f);
 if(read!=out.size())return {};return out;
}
void text(DrawList& d,const Font& f,unsigned a,const std::string& s,float x,float y,float sz,Color c=white,Align al=Align::left){d.text(f,a,s,x,y,sz,c,al);}
std::string cut(const Font& f,const std::string& s,float max,float size){return f.fit(s,size,max);}
void tile(DrawList& d,float x,float y,float w,float h,bool sel){
 d.bordered_rect({x,y,w,h},16,Color::rgb(sel?0x10365a:0x081626),sel?3:1,sel?blue:edge);
 if(sel)d.glow({x,y,w,h},16,8,blue.with_alpha(.25f));
}
const char* pages[]={"Home","Search","Trending","Favorites","Downloads","History","Settings"};
const char* symbols[]={"H","?","^","*","v","O","#"};
const std::string keys="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -_."; // 40 keys
struct State {
 int page=0,focus=0,nav=0,selected=0,offset=0,kbd=0;
 bool keyboard=false,inspect=false;
 std::string query="PS5 gaming",notice="Search videos using Triangle or open Search.";
 pstube::Video active;
 std::vector<std::string> history;
};
void write_help(DrawList& d,const Font& f,unsigned a,State& s){
 std::string help=s.keyboard?"D-PAD Move   X Type   SQUARE Delete   TRIANGLE Search   CIRCLE Cancel":
 s.inspect?"CIRCLE Back   SQUARE Favourite   TRIANGLE Download (unavailable)":
 "D-PAD Move   X Select   SQUARE Favourite   TRIANGLE Search   L1/R1 Pages   CIRCLE Back";
 d.rounded_rect({0,1010,1920,70},0,Color::rgb(0x061427));
 text(d,f,a,help,965,1050,22,dim,Align::center);
}
void result_grid(DrawList& d,const Font& f,unsigned a,const std::vector<pstube::Video>& videos,const State& s,const pstube::Service& svc){
 if(videos.empty()){text(d,f,a,"No videos available",1080,446,34,white,Align::center);text(d,f,a,"Open Search and enter a query; public API instances may be unavailable.",1080,493,22,dim,Align::center);return;}
 d.push_clip({405,240,1485,737});
 const int columns=3;
 for(int i=s.offset;i<(int)videos.size()&&i<s.offset+9;i++){
  const int k=i-s.offset;float x=418+(k%columns)*490,y=272+(k/columns)*228;
  if(y>940)break;
  tile(d,x,y,466,206,i==s.selected);
  d.gradient_rect({x+12,y+12,442,118},8,Color::rgb(0x183e77),Color::rgb(0x061225));
  d.rounded_rect({x+183,y+43,76,48},13,Color::rgb(0x0062c0,.8f));
  d.triangle({x+209,y+52,33,29},white,0,1.5708f);
  if(svc.is_favourite(videos[(size_t)i].id))text(d,f,a,"*",x+430,y+43,36,green,Align::right);
  char time[32];std::snprintf(time,sizeof(time),"%d:%02d",videos[(size_t)i].length/60,videos[(size_t)i].length%60);
  text(d,f,a,time,x+440,y+122,19,white,Align::right);
  text(d,f,a,cut(f,videos[(size_t)i].title,435,23),x+16,y+157,23);
  text(d,f,a,cut(f,videos[(size_t)i].author,395,18),x+16,y+187,18,dim);
 }
 d.pop_clip();
}
void keyboard(DrawList& d,const Font& f,unsigned a,State& s){
 d.rounded_rect({330,140,1250,825},25,Color::rgb(0x08182b));
 text(d,f,a,"Search PSTUBE",383,221,43);
 d.bordered_rect({380,254,1150,80},12,panel,2,blue);
 text(d,f,a,cut(f,s.query+"_",1075,34),407,308,34);
 text(d,f,a,"Use D-Pad and Cross to type. Triangle submits your search.",385,373,20,dim);
 for(int i=0;i<(int)keys.size();i++){
  int col=i%10,row=i/10;float x=388+col*113,y=411+row*113;
  tile(d,x,y,103,92,i==s.kbd);
  text(d,f,a,std::string(1,keys[(size_t)i]),x+51,y+59,35,white,Align::center);
 }
 text(d,f,a,"SQUARE Backspace        TRIANGLE Search        CIRCLE Close",950,939,23,dim,Align::center);
}
}
int main(){
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 Renderer renderer;if(!renderer.init())hui::sys::park();
 Font font;auto bytes=load("/app0/assets/fonts/inter-semibold.huifont");
 if(bytes.empty())bytes=load("/data/homebrew/PPSA99762/assets/fonts/inter-semibold.huifont");
 if(!font.load(bytes))hui::sys::park();
 unsigned atlas=renderer.batch().create_font_texture(font);
 hui::ps5::Pad pad;pad.open();pad.set_light_bar(0,125,230);
 pstube::Service service;State s;unsigned prev=0;int repeat=0;
 for(;;){
  std::array<hui::PadSample,64> samples{};size_t n=pad.read(samples);
  unsigned pressed=0,now=prev;
  for(size_t i=0;i<n;++i){pressed|=samples[i].buttons&~now;now=samples[i].buttons;}
  prev=now;if(--repeat<=0){if(now&hui::pad_bits::kUp)pressed|=hui::pad_bits::kUp;if(now&hui::pad_bits::kDown)pressed|=hui::pad_bits::kDown;if(now&hui::pad_bits::kLeft)pressed|=hui::pad_bits::kLeft;if(now&hui::pad_bits::kRight)pressed|=hui::pad_bits::kRight;repeat=12;}
  auto snapshot=service.snapshot();
  std::vector<pstube::Video> videos=snapshot.videos;
  if(s.page==3){videos.erase(std::remove_if(videos.begin(),videos.end(),[&](const auto& v){return !service.is_favourite(v.id);}),videos.end());}
  s.selected=std::clamp(s.selected,0,std::max(0,(int)videos.size()-1));
  if(pressed&hui::pad_bits::kCircle){
   if(s.keyboard)s.keyboard=false;
   else if(s.inspect)s.inspect=false;
   else if(s.focus==1)s.focus=0;
   else if(s.page!=0){s.page=0;s.selected=0;}
   else {pad.close();renderer.release();display.close();hui::sys::quit();}
  }
  else if(s.keyboard){
   if(pressed&hui::pad_bits::kLeft)s.kbd=(s.kbd+keys.size()-1)%keys.size();
   if(pressed&hui::pad_bits::kRight)s.kbd=(s.kbd+1)%keys.size();
   if(pressed&hui::pad_bits::kUp)s.kbd=std::max(0,s.kbd-10);
   if(pressed&hui::pad_bits::kDown)s.kbd=std::min((int)keys.size()-1,s.kbd+10);
   if((pressed&hui::pad_bits::kCross)&&s.query.size()<90)s.query+=keys[(size_t)s.kbd];
   if((pressed&hui::pad_bits::kSquare)&&!s.query.empty())s.query.pop_back();
   if(pressed&hui::pad_bits::kTriangle){if(service.search(s.query)){s.keyboard=false;s.page=1;s.focus=1;s.offset=s.selected=0;}}
  }else if(s.inspect){
   if(pressed&hui::pad_bits::kSquare){service.toggle_favourite(s.active.id);s.notice="Favourite list updated";}
   if(pressed&hui::pad_bits::kTriangle){service.download(s.active);s.notice="Downloading is not yet supported";}
  }else{
   if(pressed&hui::pad_bits::kL1){s.page=(s.page+6)%7;s.focus=0;s.selected=s.offset=0;}
   if(pressed&hui::pad_bits::kR1){s.page=(s.page+1)%7;s.focus=0;s.selected=s.offset=0;}
   if(pressed&hui::pad_bits::kTriangle){s.keyboard=true;s.page=1;s.focus=1;}
   if(s.focus==0){
    if(pressed&hui::pad_bits::kUp)s.nav=std::max(0,s.nav-1);
    if(pressed&hui::pad_bits::kDown)s.nav=std::min(6,s.nav+1);
    if(pressed&hui::pad_bits::kRight)s.focus=1;
    if(pressed&hui::pad_bits::kCross){s.page=s.nav;s.selected=s.offset=0;if(s.page==1)s.keyboard=true;}
   }else{
    if(pressed&hui::pad_bits::kLeft){if(s.selected%3==0)s.focus=0;else --s.selected;}
    if(pressed&hui::pad_bits::kRight)s.selected=std::min(std::max(0,(int)videos.size()-1),s.selected+1);
    if(pressed&hui::pad_bits::kUp)s.selected=std::max(0,s.selected-3);
    if(pressed&hui::pad_bits::kDown)s.selected=std::min(std::max(0,(int)videos.size()-1),s.selected+3);
    s.offset=(s.selected/9)*9;
    if(pressed&hui::pad_bits::kSquare){if(!videos.empty()){service.toggle_favourite(videos[(size_t)s.selected].id);s.notice="Favourites updated";}}
    if(pressed&hui::pad_bits::kCross){if(s.page==1&&videos.empty())s.keyboard=true;else if(!videos.empty()){s.active=videos[(size_t)s.selected];s.inspect=true;s.history.push_back(s.active.id);}}
   }
  }
  DrawList d;d.gradient_rect({0,0,1920,1080},0,Color::rgb(0x020916),Color::rgb(0x041d38));
  for(int i=0;i<6;i++)d.arc(165,858,280+i*42,2,4.5f,6.5f,blue.with_alpha(.12f));
  d.gradient_rect({0,0,1920,140},0,Color::rgb(0x06182d),Color::rgb(0x020c19));
  d.glow({42,26,94,88},16,16,blue.with_alpha(.7f));d.bordered_rect({42,26,94,88},19,panel,3,blue);
  text(d,font,atlas,"P",89,90,66,blue,Align::center);
  text(d,font,atlas,"PSTUBE",158,79,51,white);
  text(d,font,atlas,"by YAHBALLON",162,112,23,dim);
  tile(d,466,37,892,76,false);text(d,font,atlas,"Search: "+cut(font,s.query,780,24),499,84,24,dim);
  text(d,font,atlas,"v0.1.0",1850,84,25,white,Align::right);
  d.rounded_rect({0,140,374,870},0,Color::rgb(0x041226));
  for(int i=0;i<7;i++){float y=180+i*112;
   if(s.page==i||s.focus==0&&s.nav==i)tile(d,31,y,326,88,s.focus==0&&s.nav==i);
   text(d,font,atlas,symbols[i],65,y+58,38,i==s.page?blue:dim);
   text(d,font,atlas,pages[i],130,y+56,29,s.page==i?white:dim);
  }
  text(d,font,atlas,s.page==0?"Discover":pages[s.page],418,205,43);
  if(s.page==4){
   text(d,font,atlas,"Downloads are planned, but no verified media backend is included in this alpha.",430,320,26,dim);
   text(d,font,atlas,"PSTUBE will not show fake completed downloads.",430,370,23,dim);
  }else if(s.page==6){
   text(d,font,atlas,"Search provider: "+service.instance(),430,310,29);
   text(d,font,atlas,"Public Invidious API; availability and rate limits vary.",430,365,23,dim);
   text(d,font,atlas,"Local favourites: /download0/pstube.txt or /data/pstube/state.txt",430,425,22,dim);
  }else if(s.page==5){
   text(d,font,atlas,"Opened videos this session: "+std::to_string(s.history.size()),430,315,26,dim);
  }else{
   if(s.page==0)text(d,font,atlas,"Search first to load real videos. No fabricated recommendations.",420,255,22,dim);
   result_grid(d,font,atlas,videos,s,service);
   if(snapshot.searching)text(d,font,atlas,"Loading live results...",1710,207,23,blue,Align::right);
   else text(d,font,atlas,snapshot.status,1850,206,21,dim,Align::right);
  }
  text(d,font,atlas,cut(font,s.notice,1250,20),423,989,20,dim);
  if(s.inspect){
   d.rounded_rect({398,227,1466,710},24,Color::rgb(0x061628));
   d.bordered_rect({425,270,1410,625},22,panel,2,edge);
   text(d,font,atlas,cut(font,s.active.title,1290,43),465,345,43);
   text(d,font,atlas,s.active.author,465,405,27,dim);
   text(d,font,atlas,"VIDEO DETAILS",465,490,27,blue);
   text(d,font,atlas,"SQUARE: add/remove favourite",465,566,26,white);
   text(d,font,atlas,"Streaming playback and downloads are not implemented in v0.1.0.",465,628,27,dim);
   text(d,font,atlas,"CIRCLE: return to search results",465,700,24,dim);
  }
  if(s.keyboard)keyboard(d,font,atlas,s);
  write_help(d,font,atlas,s);
  renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
