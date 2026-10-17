// SPDX-License-Identifier: GPL-3.0-or-later
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "library.hpp"
#include "audio_player.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include "stb_image.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <map>
#include <string>
#include <sys/stat.h>
using namespace hui::gfx;
namespace {
constexpr const char* kVersion="0.1.0";
Color c(uint32_t n,float alpha=1.f){return Color::rgb(n,alpha);}
std::string readfile(const std::string& path,size_t max=12000000){
 FILE* f=fopen(path.c_str(),"rb");if(!f)return {};
 if(fseek(f,0,SEEK_END)){fclose(f);return {};}
 const long n=ftell(f);if(n<=0||(unsigned long)n>max){fclose(f);return {};}
 rewind(f);std::string s((size_t)n,0);
 if(fread(s.data(),1,s.size(),f)!=(size_t)n)s.clear();
 fclose(f);return s;
}
std::string cut(std::string s,size_t n){
 if(s.size()>n){s.resize(n-3);s+="...";}return s;
}
void txt(DrawList& d,const Font& f,unsigned atlas,const std::string& str,float x,float y,float size,Color color=c(0xffffff),Align align=Align::left){
 d.text(f,atlas,str,x,y,size,color,align);
}
struct Picture {unsigned id=0;int w=0,h=0;bool checked=false;};
void drop(Picture& p){if(p.id){GLuint id=p.id;glDeleteTextures(1,&id);p.id=0;}}
Picture load(Renderer& renderer,const std::string& path){
 Picture p;p.checked=true;
 if(path.empty())return p;
 std::string bytes=readfile(path);if(bytes.empty())return p;
 int w=0,h=0,ch=0;
 if(!stbi_info_from_memory((const stbi_uc*)bytes.data(),(int)bytes.size(),&w,&h,&ch))return p;
 if(w<1||h<1||w>4096||h>4096||(uint64_t)w*h>9000000)return p;
 stbi_uc* rgba=stbi_load_from_memory((const stbi_uc*)bytes.data(),(int)bytes.size(),&w,&h,&ch,4);
 if(!rgba)return p;
 p.id=renderer.batch().create_texture(w,h,rgba);p.w=w;p.h=h;
 stbi_image_free(rgba);return p;
}
void photo(DrawList& d,const Picture& p,float x,float y,float w,float h,bool crop){
 if(!p.id)return;
 float u=0,v=0,uw=1,vh=1;
 if(crop){
  const float source=(float)p.w/p.h,dest=w/h;
  if(source>dest){uw=dest/source;u=(1-uw)*0.5f;}
  else{vh=source/dest;v=(1-vh)*0.5f;}
 }
 d.image(p.id,{x,y,w,h},{u,v,uw,vh},c(0xffffff),2);
}
std::string timer(double t) {
 const int n=(int)std::max(0.0,t);char b[32];
 snprintf(b,sizeof(b),"%d:%02d",n/60,n%60);return b;
}
void button_bar(DrawList& d,const Font& f,unsigned at,bool playing){
 d.rounded_rect({0,996,1920,84},0,c(0x050609,0.93f));
 d.line(0,996,1920,996,1,c(0x6d737c,0.4f));
 txt(d,f,at,playing?
 "X Pause/Resume     L1/R1 Track     LEFT/RIGHT Seek     O Albums     OPTIONS Settings":
 "LEFT/RIGHT Browse     L2/R2 Artist     X Play     TRIANGLE Tracks     SQUARE Rescan     OPTIONS Settings     O Quit",
 960,1045,21,c(0xffffff),Align::center);
}
} // namespace
int main(){
 mkdir("/data/PSJukebox",0755);
 mkdir("/data/PSJukebox/Music",0755);
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 Renderer renderer;if(!renderer.init())hui::sys::park();
 Font font;
 std::string fontBytes=readfile("/app0/assets/fonts/inter-semibold.huifont");
 if(fontBytes.empty())fontBytes=readfile("/data/homebrew/PPSA99947/assets/fonts/inter-semibold.huifont");
 if(!font.load(fontBytes))hui::sys::park();
 auto at=renderer.batch().create_font_texture(font);
 hui::ps5::Pad pad;pad.open();
 auto library=jukebox::scan(jukebox::music_roots());
 std::map<std::string,Picture> texture;
 jukebox::Player player; // shared SDL2/FFmpeg player, native PS5 audio
 size_t ai=0,bi=0,ti=0;std::string currentPath;
 bool playing=false,trackList=false,settings=false,quit=false,exitConfirm=false;
 unsigned held=0;int repeat=0,frame=0;
 std::string message=library.report;
 auto clearPictures=[&](){for(auto& item:texture)drop(item.second);texture.clear();};
 auto fetch=[&](const std::string& path)->const Picture&{
  auto it=texture.find(path);
  if(it==texture.end())it=texture.emplace(path,load(renderer,path)).first;
  return it->second;
 };
 auto selectedArtist=[&]()->const jukebox::Artist* {
  return ai<library.artists.size()?&library.artists[ai]:nullptr;
 };
 auto selectedAlbum=[&]()->const jukebox::Album* {
  const auto* artist=selectedArtist();
  return artist&&bi<artist->albums.size()?&artist->albums[bi]:nullptr;
 };
 auto openSong=[&](size_t index){
  const auto* album=selectedAlbum();if(!album||index>=album->tracks.size())return;
  ti=index;currentPath=album->tracks[ti].path;
  if(player.start(currentPath)) {playing=true;message="Opening "+album->tracks[ti].name;}
  else {playing=false;message="Unable to open music file";} 
 };
 for(;;){
  std::array<hui::PadSample,64> events{};size_t n=pad.read(events);
  unsigned pressed=0,now=held;
  for(size_t i=0;i<n;i++){pressed|=events[i].buttons&~now;now=events[i].buttons;}
  held=now;
  if(--repeat<=0){
   for(unsigned bit:{hui::pad_bits::kLeft,hui::pad_bits::kRight,hui::pad_bits::kUp,hui::pad_bits::kDown})
    if(now&bit)pressed|=bit;
   repeat=12;
  }
  if(exitConfirm){
   if(pressed&hui::pad_bits::kCircle)exitConfirm=false;
   if(pressed&hui::pad_bits::kCross)quit=true;
  }else if(settings){
   if(pressed&(hui::pad_bits::kCircle|hui::pad_bits::kOptions))settings=false;
  }else if(playing){
   if(pressed&hui::pad_bits::kCircle){playing=false;player.stop();}
   else {
    if(pressed&hui::pad_bits::kCross)player.toggle_pause();
    if(pressed&hui::pad_bits::kLeft)player.skip(-10);
    if(pressed&hui::pad_bits::kRight)player.skip(10);
    if(pressed&hui::pad_bits::kL1){if(ti>0)openSong(ti-1);}
    if(pressed&hui::pad_bits::kR1){const auto* a=selectedAlbum();if(a&&ti+1<a->tracks.size())openSong(ti+1);}
   }
  }else if(pressed&hui::pad_bits::kOptions)settings=true;
  else if(pressed&hui::pad_bits::kCircle){
   if(trackList)trackList=false;else exitConfirm=true;
  }else if(pressed&hui::pad_bits::kSquare){
   library=jukebox::scan(jukebox::music_roots());ai=bi=ti=0;clearPictures();message=library.report;
  }else if(const auto* artist=selectedArtist()) {
   if(pressed&hui::pad_bits::kL2){ai=ai?ai-1:library.artists.size()-1;bi=ti=0;clearPictures();}
   if(pressed&hui::pad_bits::kR2){ai=(ai+1)%library.artists.size();bi=ti=0;clearPictures();}
   if(const auto* album=selectedAlbum()) {
    if(trackList){
     if(pressed&hui::pad_bits::kUp)ti=ti?ti-1:album->tracks.size()-1;
     if(pressed&hui::pad_bits::kDown)ti=(ti+1)%album->tracks.size();
     if(pressed&hui::pad_bits::kCross)openSong(ti);
     if(pressed&hui::pad_bits::kTriangle)trackList=false;
    }else{
     if(pressed&hui::pad_bits::kLeft)bi=bi?bi-1:artist->albums.size()-1;
     if(pressed&hui::pad_bits::kRight)bi=(bi+1)%artist->albums.size();
     if(pressed&hui::pad_bits::kTriangle){trackList=true;ti=0;}
     if(pressed&hui::pad_bits::kCross)openSong(0);
    }
   }
  }
  if(quit) {
   player.stop();clearPictures();
   pad.close();renderer.release();display.close();hui::sys::quit();
  }
  const auto* artist=selectedArtist();
  const auto* album=selectedAlbum();
  const uint32_t accent=artist?artist->theme.accent:0xffc735;
  const uint32_t secondary=artist?artist->theme.secondary:0x182b2b;
  DrawList d;
  d.gradient_rect({0,0,1920,1080},0,c(secondary),c(0x07090d));
  // Background and art are streamed as selected, with a bounded texture cache.
  if(artist&&album){
   const auto& bg=fetch(jukebox::album_background(*artist,*album));
   photo(d,bg,0,0,1920,996,true);
   d.gradient_rect({0,0,1920,220},0,c(0x000000,0.85f),c(0x000000,0.0f));
   d.gradient_rect({0,700,1920,296},0,c(0x000000,0.08f),c(0x000000,0.85f));
  }
  d.rounded_rect({0,0,1920,96},0,c(0x030407,0.62f));
  if(artist){
   photo(d,fetch(artist->logo),44,9,70,72,false);
   txt(d,font,at,cut(artist->name,22),artist->logo.empty()?45:132,50,36,c(accent));
   txt(d,font,at,"Multi Album Player",artist->logo.empty()?45:132,78,20,c(0xc5c5c5));
  }else{
   txt(d,font,at,"PSJUKEBOX",52,60,36,c(accent));
   txt(d,font,at,"Multi Album Player",52,84,19);
  }
  txt(d,font,at,"ALL ALBUMS",960,58,25,c(0xffffff),Align::center);
  txt(d,font,at,std::string("by YAHBALLON  v")+kVersion,1874,52,18,c(0xffffff),Align::right);
  txt(d,font,at,artist?cut(artist->name,30)+" LIBRARY   /   All Albums":"NO MUSIC FOUND",
      84,169,23,c(accent));
  if(album){
   txt(d,font,at,std::to_string(bi+1)+" / "+std::to_string(artist->albums.size()),
       1840,170,20,c(0xffffff),Align::right);
   txt(d,font,at,cut(album->name,45),960,306,32,c(0xffffff),Align::center);
   // Center focused jacket is larger; neighbors shrink progressively.
   const int total=(int)artist->albums.size();
   for(int offset=-2;offset<=2;offset++){
    if(total<2&&offset!=0)continue;
    const int idx=((int)bi+offset%total+total)%total;
    // Skip repeated covers when fewer than five albums.
    if(total<5&&((offset<0&&-offset>=total)||(offset>0&&offset>=total)))continue;
    if(total==2&&(offset==-2||offset==2))continue;
    const auto& card=artist->albums[(size_t)idx];
    const bool focus=offset==0;
    const float w=focus?380.f:295.f,h=focus?418.f:320.f;
    const float x=960.f+offset*350.f-w/2.f,y=focus?332.f:400.f;
    if(focus)d.glow({x-7,y-7,w+14,h+14},13,8,c(accent));
    d.shadow({x,y,w,h},5,14,c(0x000000,0.8f));
    d.rounded_rect({x,y,w,h},5,c(0x12131d));
    photo(d,fetch(card.cover),x+3,y+3,w-6,h-6,true);
    d.bordered_rect({x,y,w,h},4,c(0xffffff,0.02f),focus?5:2,
      focus?c(accent):c(0xcccccc,0.8f));
    if(card.cover.empty())txt(d,font,at,cut(card.name,24),x+w/2,y+h/2,22,c(0xffffff),Align::center);
   }
   txt(d,font,at,cut(artist->name,35),960,902,22,c(0xffffff),Align::center);
   txt(d,font,at,std::to_string(album->tracks.size())+" tracks",960,947,19,c(0xdedede),Align::center);
  }else{
   txt(d,font,at,"Copy music to /data/PSJukebox/Music/Artist/Album/",960,475,25,c(0xffffff),Align::center);
   txt(d,font,at,"Or use a USB drive with /PSJukebox/Music/Artist/Album/",960,525,20,c(0xffffff),Align::center);
  }
  if(playing){
   auto status=player.snapshot();
   d.rounded_rect({250,785,1420,187},10,c(0x05070e,0.94f));
   const std::string title=album&&ti<album->tracks.size()?album->tracks[ti].name:currentPath;
   txt(d,font,at,cut(title,55),960,837,31,c(accent),Align::center);
   txt(d,font,at,status.paused?"PAUSED":status.opening?"LOADING AUDIO":status.playing?"NOW PLAYING":cut(status.message,60),
        960,866,18,c(0xffffff),Align::center);
   const float fraction=status.duration>0?std::clamp((float)(status.seconds/status.duration),0.f,1.f):0.f;
   d.rounded_rect({395,905,1130,11},5,c(0x535765));
   d.rounded_rect({395,905,1130*fraction,11},5,c(accent));
   txt(d,font,at,timer(status.seconds)+" / "+timer(status.duration),960,953,19,c(0xffffff),Align::center);
  }
  if(trackList&&!playing&&album){
   d.rounded_rect({620,235,680,660},9,c(0x06070b,0.95f));
   txt(d,font,at,"TRACK LIST",960,289,29,c(accent),Align::center);
   int lo=(int)ti-4;if(lo<0)lo=0;
   for(int i=lo;i<(int)album->tracks.size()&&i<lo+10;i++){
    float y=335+(i-lo)*53.f;
    if(i==(int)ti)d.rounded_rect({648,y-30,615,48},6,c(accent,0.25f));
    txt(d,font,at,cut(album->tracks[i].name,43),678,y,21,c(0xffffff));
   }
  }
  if(settings||exitConfirm){
   d.rounded_rect({0,0,1920,996},0,c(0x000000,0.82f));
   d.rounded_rect({454,295,1012,414},13,c(0x101724));
   txt(d,font,at,settings?"PSJUKEBOX SETTINGS":"EXIT PSJUKEBOX?",960,380,35,c(accent),Align::center);
   txt(d,font,at,settings?"L2/R2: Switch Artist   |   Left/Right: Change Album":
      "Do you want to close the music player?",960,458,23,c(0xffffff),Align::center);
   txt(d,font,at,settings?"Music: /data/PSJukebox/Music or USB PSJukebox/Music":
      "X Confirm                                   O Cancel",960,530,22,c(0xffffff),Align::center);
   if(settings)txt(d,font,at,"O Close settings",960,612,21,c(accent),Align::center);
  }
  button_bar(d,font,at,playing);
  renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
  if(++frame%200==0&&texture.size()>16){
   // Preserve selected textures; evict all and lazily refill.
   clearPictures();
  }
 }
}
