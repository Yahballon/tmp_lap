// SPDX-License-Identifier: GPL-3.0-or-later
// Audiobook Reader: metal bookshelf UI derived from the same open-source UI library as Ebook Reader.
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "audio_player.hpp"
#include "library.hpp"
#include "chapters.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
using namespace hui::gfx;
namespace {
constexpr auto steel=0x9cb4ca,white=0xe9f5ff,cyan=0x57d9fb,muted=0x8b9aab;
const Color kSteel=Color::rgb(steel),kWhite=Color::rgb(white),kCyan=Color::rgb(cyan),kMuted=Color::rgb(muted);
std::string cut(std::string s,size_t n=30){if(s.size()>n){s.resize(n-3);s+="...";}return s;}
void txt(DrawList& d,const Font& f,unsigned atlas,const std::string& t,float x,float y,float size,Color c=kWhite,Align a=Align::left){d.text(f,atlas,t,x,y,size,c,a);}
std::string read(const char* path){FILE* f=fopen(path,"rb");if(!f)return {};
 fseek(f,0,SEEK_END);long n=ftell(f);if(n<1||n>20000000){fclose(f);return {};}
 rewind(f);std::string s((size_t)n,0);fread(&s[0],1,s.size(),f);fclose(f);return s;}
std::string stamp(double seconds){int n=(int)std::max(0.,seconds);char buf[40];snprintf(buf,sizeof(buf),"%02d:%02d:%02d",n/3600,(n/60)%60,n%60);return buf;}
void bar(DrawList& d,float x,float y,float w,float h,float progress){
 d.rounded_rect({x,y,w,h},h/2,Color::rgb(0x13202d));
 d.rounded_rect({x,y,std::max(3.f,w*std::clamp(progress,0.f,1.f)),h},h/2,kCyan);
}
void panel(DrawList& d,float x,float y,float w,float h,bool focus=false){
 d.shadow({x,y,w,h},10,13,Color::rgb(0x000000,0.5f));
 d.bordered_rect({x,y,w,h},9,Color::rgb(focus?0x28394b:0x1b2936),focus?3.f:1.f,focus?kCyan:Color::rgb(0x647788));
}
void shelf(DrawList& d,float top){
 // A physically plausible brushed aluminium rail with alternating specular bands.
 d.gradient_rect({70,top,1780,49},0,Color::rgb(0xd4dde8),Color::rgb(0x4d6377));
 d.line(71,top+2,1849,top+2,3,Color::rgb(0xf0f8ff));
 for(int i=0;i<13;i++)d.line(77,top+8+i*3.1f,1841,top+8+i*3.1f,0.5f,
    Color::rgb(i%2?0x91a6ba:0x5d7388,0.25f));
 d.rounded_rect({75,top+43,1770,12},2,Color::rgb(0x0a121c));
}
void case_art(DrawList& d,const Font& f,unsigned a,const audio::Book& b,float x,float y,bool selected){
 constexpr float w=206,h=265;
 unsigned seed=5381;for(unsigned char c:b.title)seed=seed*33+c;
 const unsigned palette[]={0x26435f,0x35576b,0x526472,0x3b4363,0x38576a,0x4d5865,0x314f59};
 if(selected)d.glow({x-4,y-4,w+8,h+8},10,7,kCyan);
 d.shadow({x,y,w,h},6,13,Color::rgb(0x000000,0.65f));
 d.bordered_rect({x,y,w,h},7,Color::rgb(palette[seed%7]),selected?4:2,selected?kCyan:kSteel);
 d.gradient_rect({x+9,y+8,w-18,86},0,Color::rgb(0x3e5367),Color::rgb(palette[seed%7]));
 d.line(x+29,y+50,x+w-29,y+50,3,kCyan);
 d.line(x+40,y+64,x+w-40,y+64,1,kSteel);
 txt(d,f,a,"M4B AUDIO",x+w/2,y+105,18,kCyan,Align::center);
 txt(d,f,a,cut(b.title,17),x+w/2,y+157,22,kWhite,Align::center);
 txt(d,f,a,cut(b.author,18),x+w/2,y+183,16,kSteel,Align::center);
 // Waveform emblem in the lower half.
 for(int j=0;j<9;j++){float mag=(float)(seed>>(j%5))%21;float cx=x+55+j*12.f;d.line(cx,y+213-mag,cx,y+213+mag,3,kCyan);}
 d.rounded_rect({x+4,y+7,8,h-14},1,Color::rgb(0xdbe7f5,0.20f));
 if(b.favorite)d.star(x+w-22,y+20,10,kCyan);
 if(b.position>0&&b.duration>0)bar(d,x+16,y+h-13,w-32,5,(float)(b.position/b.duration));
}
std::vector<std::string> paths(){
 return {audio::root()+"/Books","/app0/assets/Books","/download0/Books","/download0/Audiobooks",
 "/data/audiobooks","/data/Audiobooks","/mnt/usb0/Audiobooks","/mnt/usb0/Books",
 "/mnt/usb1/Audiobooks","/mnt/usb1/Books","/mnt/usb0","/mnt/usb1"};
}
}
int main(){
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 hui::gfx::Renderer renderer;if(!renderer.init())hui::sys::park();
 Font font;std::string bytes=read("/app0/assets/fonts/inter-semibold.huifont");
 if(bytes.empty())bytes=read("/data/homebrew/PPSA99784/assets/fonts/inter-semibold.huifont");
 if(!font.load(bytes))hui::sys::park();
 auto atlas=renderer.batch().create_font_texture(font);
 hui::ps5::Pad pad;pad.open();
 std::vector<audio::Book> books;
 audio::scan(paths(),books);
 audio::Player player;
 std::vector<audio::Chapter> chapters;
 std::string message="M4B Audiobooks | Built by YAHBALLON";
 int selected=0,filter=0,scroll=0,chapterIndex=0;
 bool viewing=false,chapterMode=false,exitDialog=false,closing=false;
 bool list=false;
 unsigned held=0;int holdCooldown=0,saveTicker=0;
 auto visible=[&](){
  std::vector<size_t> result;
  for(size_t i=0;i<books.size();i++)
   if(filter==0||(filter==1&&books[i].position>0)||(filter==2&&books[i].favorite))result.push_back(i);
  return result;
 };
 auto snapshotProgress=[&](){
  if(viewing && selected>=0){
   auto indexes=visible();
   if(selected<(int)indexes.size()){
    auto& book=books[indexes[selected]];
    auto state=player.snapshot();
    if(state.duration>0)book.duration=state.duration;
    if(state.seconds>0)book.position=state.seconds;
   }
  }
 };
 for(;;){
  std::array<hui::PadSample,64> events{};auto n=pad.read(events);
  unsigned pressed=0,now=held;
  for(size_t i=0;i<n;i++){pressed|=events[i].buttons&~now;now=events[i].buttons;}
  held=now;
  if(--holdCooldown<=0){
   if(now&hui::pad_bits::kUp)pressed|=hui::pad_bits::kUp;
   if(now&hui::pad_bits::kDown)pressed|=hui::pad_bits::kDown;
   if(now&hui::pad_bits::kLeft)pressed|=hui::pad_bits::kLeft;
   if(now&hui::pad_bits::kRight)pressed|=hui::pad_bits::kRight;
   holdCooldown=12;
  }
  auto ids=visible();
  selected=std::clamp(selected,0,std::max(0,(int)ids.size()-1));
  if(exitDialog){
   if(pressed&hui::pad_bits::kCircle)exitDialog=false;
   else if(pressed&hui::pad_bits::kCross){exitDialog=false;closing=true;}
  }else if(pressed&hui::pad_bits::kCircle){
   if(chapterMode)chapterMode=false;
   else if(viewing){snapshotProgress();player.stop();viewing=false;audio::save(books);}
   else exitDialog=true;
  }else if(viewing){
   auto status=player.snapshot();
   if(pressed&hui::pad_bits::kCross)player.toggle_pause();
   if(pressed&hui::pad_bits::kLeft)player.skip(-15);
   if(pressed&hui::pad_bits::kRight)player.skip(30);
   if(pressed&hui::pad_bits::kL2)player.skip(-60);
   if(pressed&hui::pad_bits::kR2)player.skip(60);
   if(pressed&hui::pad_bits::kTriangle)chapterMode=!chapterMode;
   if(chapterMode){
    if(pressed&hui::pad_bits::kDown)chapterIndex=std::min((int)chapters.size()-1,chapterIndex+1);
    if(pressed&hui::pad_bits::kUp)chapterIndex=std::max(0,chapterIndex-1);
    if((pressed&hui::pad_bits::kSquare)&&!chapters.empty())player.seek(chapters[chapterIndex].start);
   }else{
    if((pressed&hui::pad_bits::kR1)&&!chapters.empty()){
     for(const auto& ch:chapters)if(ch.start>status.seconds+0.2){player.seek(ch.start);break;}
    }
    if((pressed&hui::pad_bits::kL1)&&!chapters.empty()){
     double previous=0;for(const auto& ch:chapters)if(ch.start<status.seconds-2)previous=ch.start;
     player.seek(previous);
    }
   }
   if(status.ended&&!status.playing){snapshotProgress();player.stop();viewing=false;audio::save(books);message=status.message;}
   if(++saveTicker>=280){saveTicker=0;snapshotProgress();audio::save(books);}
  }else{
   if(pressed&hui::pad_bits::kOptions){audio::scan(paths(),books);selected=0;message="Library refreshed";}
   if(pressed&hui::pad_bits::kR1){filter=(filter+1)%3;selected=0;}
   if(pressed&hui::pad_bits::kL1){filter=(filter+2)%3;selected=0;}
   if(pressed&hui::pad_bits::kR2)list=!list;
   int step=list?1:7;
   if(pressed&hui::pad_bits::kUp)selected=std::max(0,selected-step);
   if(pressed&hui::pad_bits::kDown)selected=std::min((int)ids.size()-1,selected+step);
   if(pressed&hui::pad_bits::kLeft)selected=std::max(0,selected-1);
   if(pressed&hui::pad_bits::kRight)selected=std::min((int)ids.size()-1,selected+1);
   if((pressed&hui::pad_bits::kSquare)&&!ids.empty()){books[ids[selected]].favorite=!books[ids[selected]].favorite;audio::save(books);}
   if((pressed&hui::pad_bits::kCross)&&!ids.empty()){
    auto& b=books[ids[selected]];double duration=b.duration;
    chapters=audio::chapters(b.path,&duration,&b.author,&b.title);
    if(duration>0)b.duration=duration;
    chapterIndex=0;
    for(size_t i=0;i<chapters.size();i++)if(chapters[i].start<=b.position)chapterIndex=(int)i;
    if(player.start(b.path,b.position)){viewing=true;chapterMode=false;message="";}
    else message="Unable to open M4B from selected storage path";
   }
  }
  if(closing){snapshotProgress();player.stop();audio::save(books);pad.close();renderer.release();display.close();hui::sys::quit();}
  DrawList d;
  d.gradient_rect({0,0,1920,1080},0,Color::rgb(0x172533),Color::rgb(0x070e18));
  d.gradient_rect({0,0,1920,94},0,Color::rgb(0x536b82),Color::rgb(0x152536));
  d.line(0,92,1920,92,4,kCyan);
  d.bordered_rect({24,19,58,57},10,Color::rgb(0x1c3545),2,kSteel);
  for(int i=0;i<5;i++)d.line(35+i*8,49-(i%3)*7,35+i*8,55+(i%3)*7,3,kCyan);
  txt(d,font,atlas,"AUDIOBOOK READER by YAHBALLON",109,58,31,kWhite);
  txt(d,font,atlas,"v0.1.0",1878,58,22,kCyan,Align::right);
  if(viewing){
   auto st=player.snapshot();
   const auto& b=books[ids[selected]];
   txt(d,font,atlas,"NOW PLAYING",95,163,36,kWhite);
   txt(d,font,atlas,cut(b.title,75),95,222,32,kWhite);
   panel(d,80,255,590,631);
   case_art(d,font,atlas,b,270,313,false);
   txt(d,font,atlas,cut(b.author,45),375,727,22,kSteel,Align::center);
   panel(d,714,255,1110,631);
   txt(d,font,atlas,st.opening?"LOADING AUDIO...":st.paused?"PAUSED":st.playing?"PLAYING":"STOPPED",770,329,28,kCyan);
   txt(d,font,atlas,stamp(st.seconds)+"  /  "+stamp(st.duration),770,414,42,kWhite);
   bar(d,770,455,978,20,st.duration>0?(float)(st.seconds/st.duration):0.f);
   txt(d,font,atlas,"M4B | Native AAC decoder | Resume enabled",771,521,19,kSteel);
   if(chapterMode){
    txt(d,font,atlas,"CHAPTER LIST",770,580,23,kCyan);
    int begin=std::max(0,chapterIndex-4);
    for(int i=begin;i<(int)chapters.size()&&i<begin+5;i++){
      const float y=615+(i-begin)*48;
      if(i==chapterIndex)d.rounded_rect({763,y-25,985,45},5,Color::rgb(0x31475a));
      txt(d,font,atlas,cut(chapters[i].title,56),783,y+4,19,i==chapterIndex?kCyan:kWhite);
      txt(d,font,atlas,stamp(chapters[i].start),1730,y+4,17,kSteel,Align::right);
    }
   }else{
    txt(d,font,atlas,"CHAPTER "+std::to_string(chapterIndex+1)+" / "+std::to_string(chapters.size()),770,605,23,kCyan);
    txt(d,font,atlas,chapters.empty()?"No embedded chapters":cut(chapters[chapterIndex].title,57),770,652,24,kWhite);
    txt(d,font,atlas,cut(st.message,70),770,748,20,kMuted);
   }
   txt(d,font,atlas,"X Play/Pause  LEFT -15s  RIGHT +30s  L2/R2 +/-60s  L1/R1 Chapter  TRIANGLE List  O Library",960,1015,20,kWhite,Align::center);
   if(chapterMode)txt(d,font,atlas,"UP/DOWN Choose chapter  SQUARE Jump to chapter",960,1050,17,kCyan,Align::center);
  }else{
   txt(d,font,atlas,"MY AUDIOBOOK LIBRARY",84,162,37,kWhite);
   txt(d,font,atlas,std::to_string(books.size())+" audiobooks",590,160,21,kMuted);
   const char* filters[]={"All","In Progress","Favorites"};
   for(int i=0;i<3;i++){float x=1195+i*218.f;panel(d,x,124,200,66,i==filter);txt(d,font,atlas,filters[i],x+100,166,20,i==filter?kCyan:kWhite,Align::center);}
   d.rounded_rect({63,228,1797,750},6,Color::rgb(0x0b1622));
   for(int row=0;row<2;row++)shelf(d,592+row*356);
   const int count=(int)ids.size(),perPage=list?9:14;
   scroll=count?selected/perPage:0;
   for(int i=scroll*perPage;i<count&&i<(scroll+1)*perPage;i++){
    const audio::Book& b=books[ids[i]];
    const int cell=i-scroll*perPage;
    if(list){
     const float y=254+cell*77.f;
     panel(d,88,y,1747,68,i==selected);
     txt(d,font,atlas,cut(b.title,60),112,y+43,25,kWhite);
     txt(d,font,atlas,stamp(b.position)+" / "+stamp(b.duration),1800,y+42,20,kCyan,Align::right);
    }else{
     const int row=cell/7,col=cell%7;
     const float x=117+col*248.f,y=284+row*354.f;
     case_art(d,font,atlas,b,x,y,i==selected);
    }
   }
   if(count==0){txt(d,font,atlas,"No M4B audiobooks found.",960,455,30,kWhite,Align::center);
    txt(d,font,atlas,"Copy .m4b files to /data/AudioBookReader/Books or /download0/Books",960,510,21,kSteel,Align::center);}
   txt(d,font,atlas,"PAGE "+std::to_string(scroll+1)+" / "+std::to_string(std::max(1,(count+perPage-1)/perPage)),87,1009,18,kSteel);
   txt(d,font,atlas,"X Play  L1/R1 Filter  SQUARE Favourite  OPTIONS Refresh  R2 View  O Exit",960,1052,20,kWhite,Align::center);
  }
  if(!message.empty())txt(d,font,atlas,cut(message,85),960,1000,17,kCyan,Align::center);
  if(exitDialog){
   d.rounded_rect({0,100,1920,980},0,Color::rgb(0x030810,0.81f));
   panel(d,528,347,865,359,true);
   txt(d,font,atlas,"EXIT AUDIOBOOK READER?",960,439,36,kWhite,Align::center);
   txt(d,font,atlas,"Your listening progress will be saved.",960,507,24,kSteel,Align::center);
   txt(d,font,atlas,"X YES                             O NO",960,601,28,kCyan,Align::center);
  }
  renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
