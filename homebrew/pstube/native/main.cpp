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
#include <unistd.h>
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
std::string clock_time(double seconds){
 const int total=std::max(0,(int)seconds);
 const int hour=total/3600,min=(total/60)%60,sec=total%60;
 char buf[32];
 if(hour)std::snprintf(buf,sizeof(buf),"%d:%02d:%02d",hour,min,sec);
 else std::snprintf(buf,sizeof(buf),"%02d:%02d",min,sec);
 return buf;
}
std::string quality_name(int q){
 switch(q){case 360:return "360p (data saver)";case 720:return "720p HD";
 case 1080:return "1080p Full HD";default:return "Auto (up to 720p)";}
}
int next_quality(int q){
 switch(q){case 0:return 360;case 360:return 720;case 720:return 1080;default:return 0;}
}
constexpr int kGridVisible=6;
int grid_offset(int selected,int count){
 if(count<=kGridVisible)return 0;
 return std::min((std::max(0,selected)/kGridVisible)*kGridVisible,count-kGridVisible);
}
struct State {
 int page=0,focus=0,nav=0,selected=0,offset=0,kbd=0;
 bool keyboard=false,inspect=false,playing=false,playerStarted=false,streamOpenFailed=false;
 bool fullscreen=false;int playerHudFrames=0;
 std::string streamErrorText;
 std::string query="PS5 JAILBREAK NEWS",notice="Search videos using Triangle or open Search.";
 pstube::Video active;
 std::vector<std::string> history;
};
void write_help(DrawList& d,const Font& f,unsigned a,State& s){
 std::string help=s.keyboard?"D-PAD Move   X Type   SQUARE Delete   TRIANGLE Search   CIRCLE Cancel":
 s.inspect?"CIRCLE Back   SQUARE Favourite   TRIANGLE Download (unavailable)":
 "D-PAD Move   X Play Video   SQUARE Favourite   TRIANGLE Search   L1/R1 Pages   CIRCLE Back";
 d.rounded_rect({0,1010,1920,70},0,Color::rgb(0x061427));
 text(d,f,a,help,965,1050,22,dim,Align::center);
}
void result_grid(DrawList& d,const Font& f,unsigned a,const std::vector<pstube::Video>& videos,const State& s,const pstube::Service& svc,Renderer& renderer,std::map<std::string,unsigned>& images){
 if(videos.empty()){text(d,f,a,"No videos received",1080,444,34,white,Align::center);text(d,f,a,"Press Triangle to search or check your network and date/time.",1080,502,24,dim,Align::center);return;}
 d.push_clip({405,240,1485,704});
 const int columns=3;
 for(int i=s.offset;i<(int)videos.size()&&i<s.offset+6;i++){
  const int k=i-s.offset;float x=418+(k%columns)*490,y=259+(k/columns)*332;
  if(y>940)break;
  tile(d,x,y,466,321,i==s.selected);
  d.gradient_rect({x+12,y+12,442,248},8,Color::rgb(0x183e77),Color::rgb(0x061225));
  const std::string id=videos[(size_t)i].id;
  auto img=images.find(id);
  if(img==images.end()&&images.size()<120){
   std::string file=pstube::thumbnail_file(id);
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
  if(img!=images.end())d.image(img->second,{x+12,y+12,442,248},kFullUv,white,8);
  d.rounded_rect({x+183,y+109,76,48},13,Color::rgb(0x0062c0,.8f));
  d.triangle({x+209,y+118,33,29},white,0,1.5708f);
  if(svc.is_favourite(videos[(size_t)i].id))text(d,f,a,"*",x+430,y+38,36,green,Align::right);
  char time[32];std::snprintf(time,sizeof(time),"%d:%02d",videos[(size_t)i].length/60,videos[(size_t)i].length%60);
  text(d,f,a,time,x+440,y+252,19,white,Align::right);
  text(d,f,a,cut(f,videos[(size_t)i].title,435,23),x+16,y+284,23);
  text(d,f,a,cut(f,videos[(size_t)i].author,395,18),x+16,y+308,18,dim);
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
 service.search(s.query);
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
  s.selected=std::clamp(s.selected,0,s.page==6?1:std::max(0,(int)videos.size()-1));
  s.offset=grid_offset(s.selected,(int)videos.size());
  if(s.playing){
   // Options toggles true edge-to-edge video. Directional and shoulder inputs
   // queue relative seeks; decoding is performed off the rendering thread.
   if(pressed&hui::pad_bits::kOptions){
    s.fullscreen=!s.fullscreen;s.playerHudFrames=180;
   }
   int seekSeconds=0;
   if(pressed&hui::pad_bits::kLeft)seekSeconds-=10;
   if(pressed&hui::pad_bits::kRight)seekSeconds+=10;
   if(pressed&hui::pad_bits::kL1)seekSeconds-=30;
   if(pressed&hui::pad_bits::kR1)seekSeconds+=30;
   if(seekSeconds){
    if(player.seekRelative(seekSeconds)){
     s.playerHudFrames=180;
     s.notice=seekSeconds>0?"Fast forward "+std::to_string(seekSeconds)+"s":
                       "Rewind "+std::to_string(-seekSeconds)+"s";
    }else s.notice="Wait for video playback before seeking";
   }
   if(pressed&hui::pad_bits::kCross || pressed&hui::pad_bits::kTriangle||
      pressed&hui::pad_bits::kSquare)s.playerHudFrames=180;
   if(s.playerHudFrames>0)--s.playerHudFrames;
   if(pressed&hui::pad_bits::kCircle){
    player.close();service.cancel_stream();s.playing=false;s.fullscreen=false;s.playerStarted=false;s.streamOpenFailed=false;last_frame=0;
    if(video_texture){glDeleteTextures(1,&video_texture);video_texture=0;}
   }
   if((pressed&hui::pad_bits::kCross)&&s.playerStarted){
    if(player.snapshot().ended){
     player.close();s.playerStarted=false;s.streamOpenFailed=false;
     if(!service.start_stream(s.active))s.notice="Stream resolver still busy";
    }else player.pause();
   }
   if(pressed&hui::pad_bits::kTriangle)service.toggle_favourite(s.active);
   if((pressed&hui::pad_bits::kCross)&&!s.playerStarted&&!snapshot.streamResolving){
    s.streamOpenFailed=false;
    if(!service.start_stream(s.active))s.notice="Stream resolver busy";
   }
   if(pressed&hui::pad_bits::kSquare){
    if(service.download(s.active))s.notice="Saving video in Downloads";
    else s.notice="Download already in progress";
   }
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
    s.inspect=false;s.playing=true;s.fullscreen=false;s.playerHudFrames=180;s.playerStarted=false;s.streamOpenFailed=false;
    const std::string path=service.local_path(s.active.id);
    if(!path.empty()){s.playerStarted=player.open(path);}
    else service.start_stream(s.active);
   }
  }else{
   if(pressed&hui::pad_bits::kL1){s.page=(s.page+6)%7;s.focus=0;s.selected=s.offset=0;if(s.page==2)service.trending();}
   if(pressed&hui::pad_bits::kR1){s.page=(s.page+1)%7;s.focus=0;s.selected=s.offset=0;if(s.page==2)service.trending();}
   if(pressed&hui::pad_bits::kTriangle){s.keyboard=true;s.page=1;s.focus=1;}
   if(pressed&hui::pad_bits::kOptions){s.page=6;s.focus=1;s.selected=0;}
   if((pressed&hui::pad_bits::kSquare)&&s.page==6){
    std::string file="/app0/assets/videos/pstube-selftest.mp4";
    if(access(file.c_str(),R_OK)!=0)file="/data/homebrew/PPSA99762/assets/videos/pstube-selftest.mp4";
    s.active=pstube::Video{"SELFTEST001","Playback and Audio Self-Test","PSTUBE"};
    s.playerStarted=player.open(file);
    s.playing=s.playerStarted;s.fullscreen=false;s.playerHudFrames=180;
    if(video_texture){glDeleteTextures(1,&video_texture);video_texture=0;}last_frame=0;
    if(!s.playerStarted)s.notice="Self-test clip missing; check installed assets";
   }
   if(s.focus==0){
    if(pressed&hui::pad_bits::kUp)s.nav=std::max(0,s.nav-1);
    if(pressed&hui::pad_bits::kDown)s.nav=std::min(6,s.nav+1);
    if(pressed&hui::pad_bits::kRight)s.focus=1;
    if(pressed&hui::pad_bits::kCross){s.page=s.nav;s.selected=s.offset=0;if(s.page==1)s.keyboard=true;else if(s.page==2)service.trending();}
   }else{
    if(pressed&hui::pad_bits::kLeft){if(s.selected%3==0)s.focus=0;else --s.selected;}
    if(pressed&hui::pad_bits::kRight)s.selected=std::min(std::max(0,(int)videos.size()-1),s.selected+1);
    if(pressed&hui::pad_bits::kUp)s.selected=s.page==6?std::max(0,s.selected-1):
       std::max(0,s.selected-(s.page==4?1:3));
    if(pressed&hui::pad_bits::kDown)s.selected=s.page==6?std::min(1,s.selected+1):
       std::min(std::max(0,(int)(s.page==4?snapshot.jobs.size():videos.size())-1),s.selected+(s.page==4?1:3));
    s.offset=grid_offset(s.selected,(int)videos.size());
    if(pressed&hui::pad_bits::kSquare){if(!videos.empty()&&s.page!=4){service.toggle_favourite(videos[(size_t)s.selected]);s.notice="Favourites updated";}}
    if(pressed&hui::pad_bits::kCross){
     if(s.page==6){
      if(s.selected==0){
       const int q=next_quality(service.quality());
       service.set_quality(q);s.notice="Video quality: "+quality_name(q)+" (saved)";
      }else{
       static const char* hosts[]={"inv.nadeko.net","invidious.nerdvpn.de","yt.chocolatemoo53.com","invidious.tiekoetter.com","yewtu.be"};
       auto it=std::find_if(std::begin(hosts),std::end(hosts),[&](const char* h){return service.instance()==h;});
       size_t next=(it==std::end(hosts)?0:(size_t)(it-std::begin(hosts))+1)%5;
       service.set_instance(hosts[next]);s.notice="Provider changed to "+service.instance();
      }
     }else if(s.page==4&&!snapshot.jobs.empty()){
      std::string file=service.local_path(snapshot.jobs[(size_t)std::clamp(s.selected,0,(int)snapshot.jobs.size()-1)].id);
      if(!file.empty()){
       s.active=pstube::Video{snapshot.jobs[(size_t)std::clamp(s.selected,0,(int)snapshot.jobs.size()-1)].id,
         snapshot.jobs[(size_t)std::clamp(s.selected,0,(int)snapshot.jobs.size()-1)].title,"Downloaded"};
       s.playerStarted=player.open(file);s.playing=s.playerStarted;s.fullscreen=false;s.playerHudFrames=180;
       if(!s.playerStarted)s.notice="Media decoder could not open the file";
      }else s.notice="Download has not completed";
     }else if(s.page==1&&videos.empty())s.keyboard=true;
     else if(!videos.empty()){
      s.active=videos[(size_t)s.selected];service.record_history(s.active);
      s.inspect=false;s.playing=true;s.playerStarted=false;s.streamOpenFailed=false;
      if(video_texture){glDeleteTextures(1,&video_texture);video_texture=0;}last_frame=0;
      std::string file=service.local_path(s.active.id);
      if(!file.empty())s.playerStarted=player.open(file);
      else if(!service.start_stream(s.active))s.notice="Video resolver busy; try again";
     }
    }
   }
  }
  // Start FFmpeg as soon as the native Android resolver returns an HTTPS URL.
  // No complete MP4 download is needed; network buffering stays in libavformat.
  if(s.playing&&!s.playerStarted&&!s.streamOpenFailed){
   const std::string ready=service.local_path(s.active.id);
   if(!ready.empty())s.playerStarted=player.open(ready);
   else if(snapshot.streamId==s.active.id&&!snapshot.streamUrl.empty()){
    if(snapshot.streamAdaptive&&!snapshot.streamAudioUrl.empty())
     s.playerStarted=player.openAdaptiveStream(snapshot.streamUrl,snapshot.streamAudioUrl);
    else s.playerStarted=player.openStream(snapshot.streamUrl);
    if(!s.playerStarted){s.streamOpenFailed=true;s.streamErrorText="FFmpeg network player unavailable or TLS roots could not be opened";s.notice=s.streamErrorText;}
   }
  }
  // Fetch the next YouTube continuation only when near the bottom of a visible
  // feed, without blocking controller input or reloading existing cards.
  // Evict old GPU thumbnail textures so a 1000+ result feed keeps showing
  // real thumbnails on new pages instead of exhausting the fixed cache budget.
  if(thumb_textures.size()>42){
   for(auto it=thumb_textures.begin();it!=thumb_textures.end();){
    bool visible=false;
    for(int i=s.offset;i<(int)videos.size()&&i<s.offset+6;++i)
     if(videos[(size_t)i].id==it->first){visible=true;break;}
    if(!visible){glDeleteTextures(1,&it->second);it=thumb_textures.erase(it);}
    else ++it;
   }
  }
  if(!s.playing && !s.keyboard && s.focus==1 &&
     (s.page==0||s.page==1||s.page==2) &&
     snapshot.hasMore && !snapshot.loadingMore &&
     !videos.empty()&&s.selected+5>=(int)videos.size())
    service.load_more();
  DrawList d;d.gradient_rect({0,0,1920,1080},0,Color::rgb(0x020916),Color::rgb(0x041d38));
  for(int i=0;i<6;i++)d.arc(165,858,280+i*42,2,4.5f,6.5f,blue.with_alpha(.12f));
  d.gradient_rect({0,0,1920,140},0,Color::rgb(0x06182d),Color::rgb(0x020c19));
  d.glow({42,26,94,88},16,16,blue.with_alpha(.7f));d.bordered_rect({42,26,94,88},19,panel,3,blue);
  text(d,font,atlas,"P",89,90,66,blue,Align::center);
  text(d,font,atlas,"PSTUBE",158,79,51,white);
  text(d,font,atlas,"by YAHBALLON",162,112,23,dim);
  tile(d,466,37,892,76,false);text(d,font,atlas,"Search: "+cut(font,s.query,780,24),499,84,24,dim);
  text(d,font,atlas,"v0.7.1",1850,84,25,white,Align::right);
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
   tile(d,423,261,1410,91,s.focus==1&&s.selected==0);
   text(d,font,atlas,"Video quality",451,303,30,white);
   text(d,font,atlas,quality_name(service.quality()),1780,305,27,blue,Align::right);
   tile(d,423,373,1410,91,s.focus==1&&s.selected==1);
   text(d,font,atlas,"Fallback search provider",451,414,27,white);
   text(d,font,atlas,cut(font,service.instance(),620,23),1780,416,23,dim,Align::right);
   text(d,font,atlas,"D-PAD Up/Down: select     X: change quality or provider",434,509,24,dim);
   text(d,font,atlas,"Auto prefers HD when separate video + audio tracks are available.",434,559,22,dim);
   text(d,font,atlas,"Requested quality may fall back to a lower available stream.",434,598,22,dim);
   text(d,font,atlas,"SQUARE: Test built-in 5-second video/audio decoder",434,671,24,white);
   auto th=pstube::thumbnail_health();
   text(d,font,atlas,"Thumbnails: "+std::to_string(th.successful)+" ready / "+
     std::to_string(th.failed)+" failed / "+std::to_string(th.pending)+" pending",434,743,22,dim);
   if(th.failed)text(d,font,atlas,cut(font,th.lastError,1280,19),434,780,19,dim);
  }else{
   if(s.page==0&&videos.empty())text(d,font,atlas,"Connecting to public video services...",420,258,22,dim);
   result_grid(d,font,atlas,videos,s,service,renderer,thumb_textures);
   if((s.page==0||s.page==1||s.page==2)&&!videos.empty()){
    const int end=std::min((int)videos.size(),s.offset+kGridVisible);
    std::string counter=std::to_string(s.offset+1)+"-"+std::to_string(end)+
     " / "+std::to_string(videos.size())+(snapshot.hasMore?"+":"")+" videos";
    if(snapshot.loadingMore)counter+="  |  Loading next page...";
    else if(snapshot.hasMore)counter+="  |  Scroll down for more";
    text(d,font,atlas,counter,425,918,20,dim);
    const float track=1400.f;
    d.rounded_rect({425,941,track,5},2,edge);
    const float thumb=std::max(36.f,track*(float)kGridVisible/std::max(6,(int)videos.size()));
    const float pos=425.f+(track-thumb)*(float)s.offset/(float)std::max(1,(int)videos.size()-kGridVisible);
    d.rounded_rect({pos,941,thumb,5},2,blue);
   }
   if(snapshot.searching)text(d,font,atlas,"Loading...",1850,207,22,blue,Align::right);
   if(snapshot.loadingMore)text(d,font,atlas,"Loading more videos...",1470,990,20,blue,Align::right);
   else if(!videos.empty())text(d,font,atlas,std::to_string(videos.size())+
     (snapshot.hasMore?"+ videos":" videos"),1850,206,21,dim,Align::right);
   if(s.page==0||s.page==1||s.page==2){
    text(d,font,atlas,cut(font,snapshot.status,1420,18),430,967,18,snapshot.searching?blue:dim);
   }
  }
  text(d,font,atlas,cut(font,s.notice,1250,20),423,992,20,dim);
  if((s.page==0||s.page==1||s.page==2)&&!s.playing){
   auto th=pstube::thumbnail_health();
   text(d,font,atlas,"Thumbnails "+std::to_string(th.successful)+"/"+
      std::to_string(th.successful+th.failed+th.pending),1850,990,20,th.failed?blue:dim,Align::right);
  }
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
   const auto st=player.snapshot();
   d.rounded_rect({0,0,1920,1080},0,Color::rgb(0x000000));
   if(!s.fullscreen){
    d.gradient_rect({0,0,1920,110},0,Color::rgb(0x07172e),Color::rgb(0x020712));
    text(d,font,atlas,"PSTUBE",52,75,38,white);
    text(d,font,atlas,"NOW PLAYING",300,73,24,blue);
    text(d,font,atlas,cut(font,s.active.title,1120,26),1860,72,26,white,Align::right);
    d.bordered_rect({95,135,1730,810},22,Color::rgb(0x040e1f),2,edge);
   }
   std::vector<unsigned char> rgba;int w=0,h=0;
   if(s.playerStarted&&player.pull(rgba,w,h,last_frame)){
    if(w>0&&h>0&&(w!=texture_w||h!=texture_h||video_texture==0)){
     if(video_texture)glDeleteTextures(1,&video_texture);
     video_texture=renderer.batch().create_texture(w,h,rgba.data());
     texture_w=w;texture_h=h;
    }else if(video_texture){
     glBindTexture(GL_TEXTURE_2D,video_texture);
     glTexSubImage2D(GL_TEXTURE_2D,0,0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
    }
   }
   if(s.playerStarted&&video_texture){
    const float scale=s.fullscreen?
      std::min(1920.f/(float)texture_w,1080.f/(float)texture_h):
      std::min(1700.f/(float)texture_w,780.f/(float)texture_h);
    const float fw=texture_w*scale,fh=texture_h*scale;
    d.image(video_texture,{(1920-fw)*.5f,540-fh*.5f,fw,fh},kFullUv,white);
   }else{
    // Keep the real downloaded thumbnail visible while resolving a media URL.
    auto ti=thumb_textures.find(s.active.id);
    if(ti!=thumb_textures.end())d.image(ti->second,{545,195,830,467},kFullUv,Color::rgb(0x8b99ac),18);
    text(d,font,atlas,cut(font,s.active.title,1400,41),960,740,41,white,Align::center);
    std::string state="Connecting to video...";
    bool isFail=false;
    if(snapshot.streamId==s.active.id){
     if(!snapshot.streamError.empty()){state=snapshot.streamError;isFail=true;}
     else if(snapshot.streamResolving)state="Resolving stream URL...";
     else if(!snapshot.streamUrl.empty())state="Opening HTTPS stream...";
    }
    if(s.playerStarted){state=st.message;isFail=st.ended&&!st.playing;}
    if(s.streamOpenFailed){state=s.streamErrorText;isFail=true;}
    text(d,font,atlas,cut(font,state,1370,27),960,799,27,isFail?Color::rgb(0xff747a):dim,Align::center);
    if(!isFail){d.rounded_rect({505,839,910,11},5,edge);d.rounded_rect({505,839,190,11},5,blue);}
    text(d,font,atlas,isFail?"X Retry  SQUARE Save offline  CIRCLE Back":
      "Buffering...  SQUARE Save offline  CIRCLE Back",960,902,23,dim,Align::center);
   }
   if(s.playerStarted&&st.ended)text(d,font,atlas,cut(font,st.message,1200,27),960,900,27,dim,Align::center);
   const bool showHud=!s.fullscreen||s.playerHudFrames>0||st.paused||st.ended;
   if(showHud){
    if(st.width>0&&st.height>0)
     text(d,font,atlas,std::to_string(st.height)+"p",1800,142,25,green,Align::right);
    if(s.fullscreen)d.rounded_rect({0,924,1920,156},0,Color::rgb(0x051124,.87f));
    else d.rounded_rect({0,946,1920,134},0,Color::rgb(0x06152b));
    const float y=s.fullscreen?990.f:982.f;
    const double duration=st.duration;
    if(duration>0){
     const float progress=(float)std::clamp(st.seconds/duration,0.0,1.0);
     d.rounded_rect({58,y,1804,8},4,edge);
     if(progress>0)d.rounded_rect({58,y,1804.f*progress,8},4,blue);
     text(d,font,atlas,clock_time(st.seconds)+" / "+clock_time(duration),60,y+37,21,white);
    }else text(d,font,atlas,clock_time(st.seconds)+" / --:--",60,y+37,21,white);
    text(d,font,atlas,cut(font,s.active.title,1100,23),310,y+37,23,white);
    const char* instructions="LEFT/RIGHT +/-10s    L1/R1 +/-30s    OPTIONS Fullscreen    X Pause    CIRCLE Back";
    text(d,font,atlas,instructions,1860,s.fullscreen?1060.f:1060.f,20,dim,Align::right);
    if(st.seeking)text(d,font,atlas,"Seeking...",960,915,25,blue,Align::center);
    else if(st.message.find("Seeking unavailable")!=std::string::npos)
     text(d,font,atlas,cut(font,st.message,1450,25),960,915,25,white,Align::center);
   }
  }
  if(!s.playing)write_help(d,font,atlas,s);
  renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
