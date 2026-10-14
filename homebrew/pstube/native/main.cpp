// SPDX-License-Identifier: GPL-3.0-or-later
// PSTUBE native video browser, HTTPS download manager, FFmpeg video / SDL audio.
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "pstube.hpp"
#include "media.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"
#include <map>
#include <sys/stat.h>
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
 bool keyboard=false,inspect=false,playing=false;
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
void result_grid(DrawList& d,const Font& f,unsigned a,const std::vector<pstube::Video>& videos,const State& s,const pstube::Service& svc,Renderer& renderer,std::map<std::string,unsigned>& images){
 if(videos.empty()){text(d,f,a,"No videos available",1080,446,34,white,Align::center);text(d,f,a,"Open Search and enter a query; public API instances may be unavailable.",1080,493,22,dim,Align::center);return;}
 d.push_clip({405,240,1485,737});
 const int columns=3;
 for(int i=s.offset;i<(int)videos.size()&&i<s.offset+9;i++){
  const int k=i-s.offset;float x=418+(k%columns)*490,y=272+(k/columns)*228;
  if(y>940)break;
  tile(d,x,y,466,206,i==s.selected);
  d.gradient_rect({x+12,y+12,442,118},8,Color::rgb(0x183e77),Color::rgb(0x061225));
  const std::string id=videos[(size_t)i].id;
  auto img=images.find(id);
  if(img==images.end()&&images.size()<120){
   std::string file="/data/pstube/thumbs/"+id+".jpg";
   struct stat st{};
   if(stat(file.c_str(),&st)==0&&st.st_size>1024&&st.st_size<1024*1024){
    int iw=0,ih=0,n=0;unsigned char* pixels=stbi_load(file.c_str(),&iw,&ih,&n,4);
    if(pixels&&iw>0&&ih>0&&iw<=1024&&ih<=1024){
     unsigned tex=renderer.batch().create_texture(iw,ih,pixels);
     if(tex){images[id]=tex;img=images.find(id);}
    }
    if(pixels)stbi_image_free(pixels);
   }
  }
  if(img!=images.end())d.image(img->second,{x+12,y+12,442,118},kFullUv,white,8);
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
 pstube::Service service;pstube::MediaPlayer player;State s;unsigned prev=0,stick_prev=0;int repeat=0;
 unsigned video_texture=0;int texture_w=0,texture_h=0;uint64_t last_frame=0;
 std::map<std::string,unsigned> thumb_textures;
 service.trending();
 for(;;){
  std::array<hui::PadSample,64> samples{};size_t n=pad.read(samples);
  unsigned pressed=0,now=prev;
  for(size_t i=0;i<n;++i){pressed|=samples[i].buttons&~now;now=samples[i].buttons;}
  prev=now;
  unsigned stick=0;
  if(n){const auto& last=samples[n-1];
   if(last.left_x<60)stick|=hui::pad_bits::kLeft;
   if(last.left_x>195)stick|=hui::pad_bits::kRight;
   if(last.left_y<60)stick|=hui::pad_bits::kUp;
   if(last.left_y>195)stick|=hui::pad_bits::kDown;
  }
  pressed|=stick&~stick_prev;stick_prev=stick;
  if(--repeat<=0){pressed|=stick;if(now&hui::pad_bits::kUp)pressed|=hui::pad_bits::kUp;if(now&hui::pad_bits::kDown)pressed|=hui::pad_bits::kDown;if(now&hui::pad_bits::kLeft)pressed|=hui::pad_bits::kLeft;if(now&hui::pad_bits::kRight)pressed|=hui::pad_bits::kRight;repeat=12;}
  auto snapshot=service.snapshot();
  std::vector<pstube::Video> videos=snapshot.videos;
  if(s.page==3)videos=service.favorite_videos();
  if(s.page==5)videos=service.history_videos();
  s.selected=std::clamp(s.selected,0,std::max(0,(int)videos.size()-1));
  if(s.playing){
   if(pressed&hui::pad_bits::kCircle){player.close();s.playing=false;last_frame=0;}
   if(pressed&hui::pad_bits::kCross)player.pause();
  }
  else if(pressed&hui::pad_bits::kCircle){
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
   if(pressed&hui::pad_bits::kSquare){service.toggle_favourite(s.active);s.notice="Favourite list updated";}
   if(pressed&hui::pad_bits::kTriangle){
    if(service.download(s.active))s.notice="MP4 download queued; check Downloads";
    else s.notice="Download is already active or the video ID is invalid";
   }
   if(pressed&hui::pad_bits::kCross){
    std::string file=service.local_path(s.active.id);
    if(file.empty()){if(service.download(s.active))s.notice="Downloading MP4 - select again when complete";}
    else if(player.open(file)){s.playing=true;s.notice="Starting playback";}
    else s.notice="Unable to open downloaded media";
   }
  }else{
   if(pressed&hui::pad_bits::kL1){s.page=(s.page+6)%7;s.focus=0;s.selected=s.offset=0;if(s.page==2)service.trending();}
   if(pressed&hui::pad_bits::kR1){s.page=(s.page+1)%7;s.focus=0;s.selected=s.offset=0;if(s.page==2)service.trending();}
   if(pressed&hui::pad_bits::kTriangle){s.keyboard=true;s.page=1;s.focus=1;}
   if(pressed&hui::pad_bits::kOptions){s.page=6;s.focus=1;s.selected=0;}
   if(s.focus==0){
    if(pressed&hui::pad_bits::kUp)s.nav=std::max(0,s.nav-1);
    if(pressed&hui::pad_bits::kDown)s.nav=std::min(6,s.nav+1);
    if(pressed&hui::pad_bits::kRight)s.focus=1;
    if(pressed&hui::pad_bits::kCross){s.page=s.nav;s.selected=s.offset=0;if(s.page==1)s.keyboard=true;else if(s.page==2)service.trending();}
   }else{
    if(pressed&hui::pad_bits::kLeft){if(s.selected%3==0)s.focus=0;else --s.selected;}
    if(pressed&hui::pad_bits::kRight)s.selected=std::min(std::max(0,(int)videos.size()-1),s.selected+1);
    if(pressed&hui::pad_bits::kUp)s.selected=std::max(0,s.selected-(s.page==4?1:3));
    if(pressed&hui::pad_bits::kDown)s.selected=std::min(std::max(0,(int)(s.page==4?snapshot.jobs.size():videos.size())-1),s.selected+(s.page==4?1:3));
    s.offset=(s.selected/9)*9;
    if(pressed&hui::pad_bits::kSquare){if(!videos.empty()&&s.page!=4){service.toggle_favourite(videos[(size_t)s.selected]);s.notice="Favourites updated";}}
    if(pressed&hui::pad_bits::kCross){
     if(s.page==6){static const char* hosts[]={"yewtu.be","inv.nadeko.net","invidious.tiekoetter.com","yt.chocolatemoo53.com"};
      auto it=std::find_if(std::begin(hosts),std::end(hosts),[&](const char* h){return service.instance()==h;});
      size_t next=(it==std::end(hosts)?0:(size_t)(it-std::begin(hosts))+1)%4;
      service.set_instance(hosts[next]);s.notice="Provider changed to "+service.instance();
     }else if(s.page==4&&!snapshot.jobs.empty()){
      std::string file=service.local_path(snapshot.jobs[(size_t)std::clamp(s.selected,0,(int)snapshot.jobs.size()-1)].id);
      if(!file.empty()&&player.open(file))s.playing=true;
      else s.notice="Download has not completed";
     }else if(s.page==1&&videos.empty())s.keyboard=true;
     else if(!videos.empty()){s.active=videos[(size_t)s.selected];s.inspect=true;service.record_history(s.active);s.history.push_back(s.active.id);}
    }
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
  text(d,font,atlas,"v0.2.0",1850,84,25,white,Align::right);
  d.rounded_rect({0,140,374,870},0,Color::rgb(0x041226));
  for(int i=0;i<7;i++){float y=180+i*112;
   if(s.page==i||s.focus==0&&s.nav==i)tile(d,31,y,326,88,s.focus==0&&s.nav==i);
   text(d,font,atlas,symbols[i],65,y+58,38,i==s.page?blue:dim);
   text(d,font,atlas,pages[i],130,y+56,29,s.page==i?white:dim);
  }
  text(d,font,atlas,s.page==0?"Discover":pages[s.page],418,205,43);
  if(s.page==4){
   if(snapshot.jobs.empty())text(d,font,atlas,"No downloads yet. Select a video and press Triangle.",430,320,28,dim);
   for(size_t i=0;i<snapshot.jobs.size()&&i<10;i++){
    const auto& job=snapshot.jobs[i];float y=275+(float)i*68;
    tile(d,425,y,1400,59,(int)i==s.selected&&s.focus==1);
    text(d,font,atlas,cut(font,job.title.empty()?job.id:job.title,770,22),449,y+38,22);
    text(d,font,atlas,job.state,1360,y+38,21,job.state=="Complete"?green:dim);
    char p[24];std::snprintf(p,sizeof(p),"%d%%",(int)job.progress);
    text(d,font,atlas,p,1793,y+38,22,white,Align::right);
   }
  }else if(s.page==6){
   text(d,font,atlas,"Search provider: "+service.instance(),430,310,29);
   text(d,font,atlas,"Press Cross to change provider. Availability and rate limits vary.",430,365,23,dim);
   text(d,font,atlas,"Local favourites: /download0/pstube.txt or /data/pstube/state.txt",430,425,22,dim);
  }else{
   if(s.page==0)text(d,font,atlas,"Search first to load real videos. No fabricated recommendations.",420,255,22,dim);
   result_grid(d,font,atlas,videos,s,service,renderer,thumb_textures);
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
   text(d,font,atlas,service.local_path(s.active.id).empty()?
    "Cross: Download MP4 (available muxed formats)":"Cross: Play downloaded MP4",465,628,27,dim);
   text(d,font,atlas,"HD adaptive streams and account login are not supported yet.",465,668,23,dim);
   text(d,font,atlas,"CIRCLE: return to search results",465,700,24,dim);
  }
  if(s.keyboard)keyboard(d,font,atlas,s);
  if(s.playing){
   auto st=player.snapshot();
   std::vector<unsigned char> rgba;int w=0,h=0;
   if(player.pull(rgba,w,h,last_frame)){
    if(w>0&&h>0&&(w!=texture_w||h!=texture_h||video_texture==0)){
     if(video_texture)glDeleteTextures(1,&video_texture);
     video_texture=renderer.batch().create_texture(w,h,rgba.data());
     texture_w=w;texture_h=h;
    }else if(video_texture){
     glBindTexture(GL_TEXTURE_2D,video_texture);
     glTexSubImage2D(GL_TEXTURE_2D,0,0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
    }
   }
   d.rounded_rect({0,0,1920,1080},0,Color::rgb(0x000000));
   if(video_texture){
    float scale=std::min(1920.f/(float)texture_w,1015.f/(float)texture_h);
    float fw=texture_w*scale,fh=texture_h*scale;
    d.image(video_texture,{(1920-fw)*.5f,(1015-fh)*.5f,fw,fh},kFullUv,white);
   }
   d.rounded_rect({0,1010,1920,70},0,Color::rgb(0x07162b,.9f));
   text(d,font,atlas,cut(font,s.active.title,1300,24),55,1052,24,white);
   char t[64];std::snprintf(t,sizeof(t),"%.0fs",st.seconds);
   text(d,font,atlas,std::string(st.paused?"PAUSED ":"PLAY ")+t+"   X Pause   CIRCLE Back",1855,1052,21,dim,Align::right);
   if(st.opening||st.ended)text(d,font,atlas,st.message,960,540,34,white,Align::center);
  }
  if(!s.playing)write_help(d,font,atlas,s);
  renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
