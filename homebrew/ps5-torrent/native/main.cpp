// SPDX-License-Identifier: GPL-3.0-or-later
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "torrent_engine.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
using namespace hui::gfx;
namespace {
Color white=Color::rgb(0xe8f5fb),muted=Color::rgb(0x93aabd),mint=Color::rgb(0x46e4bd),blue=Color::rgb(0x55a7ff),dim=Color::rgb(0x192c43),edge=Color::rgb(0x28415a),gold=Color::rgb(0xf8c878);
std::string load(const char* p){FILE* f=fopen(p,"rb");if(!f)return {};if(fseek(f,0,SEEK_END)){fclose(f);return {};}long n=ftell(f);if(n<=0||n>16000000){fclose(f);return {};}rewind(f);std::string data((size_t)n,0);fread(&data[0],1,(size_t)n,f);fclose(f);return data;}
void text(DrawList& d,const Font& f,unsigned a,const std::string& s,float x,float y,int size,Color c=white,Align alignment=Align::left){d.text(f,a,s,x,y,(float)size,c,alignment);}
void card(DrawList& d,float x,float y,float w,float h,bool selected=false){d.bordered_rect({x,y,w,h},18,Color::rgb(selected?0x193e52:0x0c1929),selected?3:1,selected?mint:edge);}
void bar(DrawList& d,float x,float y,float w,float p,Color c){d.rounded_rect({x,y,w,11},5,Color::rgb(0x27394d));d.rounded_rect({x,y,w*std::clamp(p,0.f,1.f),11},5,c);}
std::string size(uint64_t bytes){char b[70];double v=(double)bytes;const char* units[]={"B","KiB","MiB","GiB","TiB"};int i=0;while(v>=1024&&i<4){v/=1024;i++;}snprintf(b,sizeof(b),"%.1f %s",v,units[i]);return b;}
std::string rate(double bps){return size((uint64_t)std::max(0.,bps))+"/s";}
std::string shorten(std::string s,size_t max){return s.size()>max?s.substr(0,max-3)+"...":s;}
}
int main(){
 hui::ps5::Display display;if(!display.open())hui::sys::park();hui::gfx::Renderer renderer;if(!renderer.init())hui::sys::park();
 Font font;std::string fontBytes=load("/app0/assets/fonts/inter-semibold.huifont");if(fontBytes.empty())fontBytes=load("/data/homebrew/PPSA99684/assets/fonts/inter-semibold.huifont");if(!font.load(fontBytes))hui::sys::park();
 unsigned atlas=renderer.batch().create_font_texture(font);hui::ps5::Pad pad;pad.open();
 auto& engine=torrent::engine();int page=0,selected=0;unsigned previous=0;std::string notice="Ready to download a legal test torrent.";auto library=torrent::discoverTorrents();
 const char* tabs[]={"Dashboard","Torrent Library","Storage & Settings","Logs","About"};
 for(;;){
  std::array<hui::PadSample,64> samples{};unsigned pressed=0;size_t n=pad.read(samples);
  for(size_t i=0;i<n;i++){pressed|=samples[i].buttons&~previous;previous=samples[i].buttons;}
  if(pressed&hui::pad_bits::kL1)page=(page+4)%5;
  if(pressed&hui::pad_bits::kR1)page=(page+1)%5;
  if(pressed&hui::pad_bits::kUp){if(page==1&&selected>0)selected--;else if(page==0)page=(page+3)%4;}
  if(pressed&hui::pad_bits::kDown){if(page==1&&selected+1<(int)library.size())selected++;else if(page==0)page=(page+1)%4;}
  if(pressed&hui::pad_bits::kCircle){if(page!=0)page=0;else{engine.stop();pad.close();renderer.release();display.close();hui::sys::quit();}}
  if(pressed&hui::pad_bits::kTriangle){library=torrent::discoverTorrents();notice="Torrent library refreshed"; }
  if(pressed&hui::pad_bits::kSquare){engine.stop();notice="Stop requested"; }
  if(pressed&hui::pad_bits::kCross){
   if(page==0||page==1){auto list=torrent::discoverTorrents();if(list.empty())notice="Test torrent is missing from package";else{int index=page==1?std::min(selected,(int)list.size()-1):0;if(engine.start(list[index])){notice="Loading "+list[index];page=0;}else notice="A transfer is already running";}}
   else if(page==2){if(engine.snapshot().running)notice="Stop transfer before switching storage";else{engine.setUsb(!engine.usingUsb());notice=engine.usingUsb()?"USB download storage selected":"Internal storage selected";}}
  }
  auto s=engine.snapshot();const float p=s.totalBytes?float((double)s.completedBytes/s.totalBytes):0;
  DrawList d;d.gradient_rect({0,0,1920,1080},0,Color::rgb(0x081624),Color::rgb(0x050e19));
  for(int i=0;i<9;i++)d.arc(1590,350,290+i*32,2,3.5f,6.25f,Color::rgb(0x23d8b0,.08f));
  d.rounded_rect({0,0,1920,102},0,Color::rgb(0x0d1b2d));d.circle(79,51,28,mint);d.circle(79,51,18,Color::rgb(0x0d1b2d));d.line(63,51,95,51,4,mint);d.line(79,35,79,67,4,mint);
  text(d,font,atlas,"TORRENT FLOW",127,55,35);
  text(d,font,atlas,"by YAHBALLON",464,54,21,mint);
  text(d,font,atlas,"v0.1.1",1830,56,23,white,Align::right);
  card(d,27,125,335,872);
  for(int i=0;i<5;i++){
   float y=149+i*104;card(d,43,y,302,88,page==i);
   text(d,font,atlas,tabs[i],66,y+41,24,page==i?mint:white);
   text(d,font,atlas,i==0?"Live transfer overview":i==1?"Bundled .torrent files":i==2?"Cache and destination":i==3?"Live transfer diagnostics":"Client and limitations",66,y+70,16,muted);
  }
  text(d,font,atlas,"NETWORK",60,700,19,mint);
  text(d,font,atlas,"Tracker: HTTP BitTorrent v1",60,746,18,muted);
  text(d,font,atlas,"Peers: TCP / IPv4",60,783,18,muted);
  text(d,font,atlas,"Integrity: SHA-1 pieces",60,821,18,muted);
  text(d,font,atlas,s.running?"TRANSFER ACTIVE":s.done?"DOWNLOAD COMPLETE":"TRANSFER IDLE",60,937,20,s.running?mint:s.done?gold:muted);
  if(page==0){
   card(d,385,126,1497,240);
   text(d,font,atlas,"TRANSFER OVERVIEW",428,181,20,mint);
   text(d,font,atlas,s.name.empty()?"Debian 13.7 netinst - ready to test":shorten(s.name,54),427,238,38);
   text(d,font,atlas,shorten(s.status,96),429,282,21,s.done?mint:muted);
   bar(d,427,321,1398,p,mint);
   text(d,font,atlas,std::to_string((int)(p*100))+"%",1825,310,26,mint,Align::right);
   const char* names[]={"DOWNLOAD SPEED","UPLOAD SPEED","VERIFIED DATA","PEERS"};
   std::string vals[]={rate(s.downBps),rate(s.upBps),size(s.completedBytes)+" / "+size(s.totalBytes),std::to_string(s.activePeers)+" / "+std::to_string(s.peerCount)};
   for(int i=0;i<4;i++){float x=387+i*378;card(d,x,387,362,152);text(d,font,atlas,names[i],x+25,430,19,muted);text(d,font,atlas,shorten(vals[i],23),x+24,492,32,i==0?mint:i==1?blue:white);}
   card(d,387,559,965,420);
   text(d,font,atlas,"TORRENT DETAILS",419,609,22,mint);
   const char* keys[]={"Name","Pieces verified","Tracker","Info hash","Destination"};
   std::string values[]={shorten(s.name.empty()?"Not loaded":s.name,52),std::to_string(s.verifiedPieces)+" / "+std::to_string(s.pieceCount),shorten(s.tracker,58),shorten(s.infoHash,42),shorten(s.downloadDir,51)};
   for(int i=0;i<5;i++){float y=668+i*57;text(d,font,atlas,keys[i],421,y,20,muted);text(d,font,atlas,values[i],652,y,21);if(i<4)d.line(419,y+15,1323,y+15,1,edge);}
   card(d,1371,559,511,420);
   text(d,font,atlas,"QUICK ACTIONS",1402,608,23,mint);
   text(d,font,atlas,"X  Start test torrent",1403,673,24);
   text(d,font,atlas,"SQUARE  Stop transfer",1403,731,23);
   text(d,font,atlas,"TRIANGLE  Refresh library",1403,790,22);
   text(d,font,atlas,"R1  Next screen",1403,847,23);
   text(d,font,atlas,"Existing verified pieces survive",1403,919,18,muted);
  }else if(page==1){
   card(d,386,125,1495,866);text(d,font,atlas,"TORRENT LIBRARY",429,206,43);
   text(d,font,atlas,"Packaged legal test torrent plus cache files",429,257,22,muted);
   if(library.empty())text(d,font,atlas,"No .torrent file found - package assets may be missing",430,370,25,gold);
   for(size_t i=0;i<library.size()&&i<12;i++){float y=302+i*101;card(d,413,y,1440,91,(int)i==selected);text(d,font,atlas,shorten(library[i],105),442,y+42,25);text(d,font,atlas,(int)i==selected?"X  Download / Resume":"Available",443,y+75,17,(int)i==selected?mint:muted);}
  }else if(page==2){
   card(d,386,125,1495,866);text(d,font,atlas,"STORAGE & SETTINGS",429,208,43);
   text(d,font,atlas,"Mode: "+std::string(s.usb?"USB 0":"Internal PS5 storage"),430,304,30,mint);
   text(d,font,atlas,"Cache directory",431,394,21,muted);text(d,font,atlas,s.cacheDir,431,442,29);
   text(d,font,atlas,"Download directory",431,525,21,muted);text(d,font,atlas,s.downloadDir,431,573,29);
   text(d,font,atlas,"X  Toggle internal / USB0 storage",431,696,27,mint);
   text(d,font,atlas,"Switch storage only when idle. Partially downloaded data stays in place.",431,762,22,muted);
   text(d,font,atlas,"Unverified pieces are never reported as completed.",431,828,22,muted);
   text(d,font,atlas,"Log file: "+shorten(torrent::logPath(),79),431,891,20,mint);
  }else if(page==3){
   card(d,386,125,1495,866);
   text(d,font,atlas,"LIVE TRANSFER LOGS",429,209,42);
   text(d,font,atlas,"Logs are saved after every connection attempt; latest lines appear here.",430,265,21,muted);
   text(d,font,atlas,"Log file: "+shorten(torrent::logPath(),84),430,319,24,mint);
   auto lines=torrent::recentLogs();
   const size_t count=std::min<size_t>(12,lines.size());
   if(!count)text(d,font,atlas,"No log messages yet.",433,405,23,muted);
   for(size_t i=0;i<count;i++){
    const std::string& line=lines[lines.size()-count+i];
    float y=380+i*48;
    if(i%2==0)d.rounded_rect({421,y-29,1417,42},7,Color::rgb(0x11263a));
    const bool failed=line.find("FAILED")!=std::string::npos||line.find("timeout")!=std::string::npos;
    text(d,font,atlas,shorten(line,121),437,y,18,failed?gold:muted);
   }
  }else{
   card(d,386,125,1495,866);text(d,font,atlas,"ABOUT TORRENT FLOW",429,210,43);
   text(d,font,atlas,"Native PS5 BitTorrent test client  /  PPSA99684",431,288,26,mint);
   const char* lines[]={"BitTorrent v1 single-file torrents, HTTP trackers, IPv4 TCP peers","Sequential 16 KiB piece requests with SHA-1 integrity checks","Resumable local storage, separate cache and downloads folders","Bundled Debian GNU/Linux netinst torrent - distribution is legal","Experimental: HTTP + Debian UDP fallback, no DHT, magnets or multi-file","A paused job stops safely after its current network operation.","BitFiend studied as a design reference; no upstream code copied."};
   for(int i=0;i<7;i++)text(d,font,atlas,lines[i],434,369+i*72,22,i==4?gold:muted);
  }
  d.rounded_rect({25,1007,1870,53},12,Color::rgb(0x102237));
  text(d,font,atlas,"D-Pad Navigate     L1/R1 Pages     X Select / Download     O Back     SQUARE Stop     TRIANGLE Refresh",45,1041,18,muted);
  text(d,font,atlas,shorten(notice,50),1849,1041,18,mint,Align::right);
  renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());display.swap();hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
