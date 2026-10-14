// SPDX-License-Identifier: GPL-3.0-or-later
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "download_manager.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <string>
#include <vector>
using namespace hui::gfx;
namespace {
Color white=Color::rgb(0xe8f5fb),muted=Color::rgb(0x93aabd),
 mint=Color::rgb(0x46e4bd),blue=Color::rgb(0x55a7ff),
 dim=Color::rgb(0x192c43),edge=Color::rgb(0x28415a),gold=Color::rgb(0xf8c878);
std::string load(const char* p){
 FILE* f=fopen(p,"rb");if(!f)return {};
 if(fseek(f,0,SEEK_END)){fclose(f);return {};}
 long n=ftell(f);if(n<=0||n>16000000){fclose(f);return {};}
 rewind(f);std::string data((size_t)n,0);fread(&data[0],1,(size_t)n,f);fclose(f);return data;
}
void text(DrawList& d,const Font& f,unsigned a,const std::string& s,float x,float y,int size,Color c=white,Align alignment=Align::left){
 d.text(f,a,s,x,y,(float)size,c,alignment);
}
void card(DrawList& d,float x,float y,float w,float h,bool selected=false){
 d.bordered_rect({x,y,w,h},18,Color::rgb(selected?0x193e52:0x0c1929),selected?3:1,selected?mint:edge);
}
void bar(DrawList& d,float x,float y,float w,float p,Color c){
 d.rounded_rect({x,y,w,10},5,Color::rgb(0x27394d));
 d.rounded_rect({x,y,w*std::clamp(p,0.f,1.f),10},5,c);
}
std::string size(uint64_t bytes){
 char b[70];double v=(double)bytes;const char* units[]={"B","KiB","MiB","GiB","TiB"};
 int i=0;while(v>=1024&&i<4){v/=1024;i++;}
 snprintf(b,sizeof(b),"%.1f %s",v,units[i]);return b;
}
std::string rate(double bps){return size((uint64_t)std::max(0.,bps))+"/s";}
std::string shorten(const std::string& s,size_t max){return s.size()>max?s.substr(0,max-3)+"...":s;}
std::string phase(const torrent::JobInfo& job){
 if(job.completedJob)return "COMPLETE";
 if(job.cancelled)return "CANCELLED";
 if(job.failed)return "ERROR";
 if(job.queued)return "QUEUED";
 return job.phase=="stopping"?"STOPPING":"DOWNLOADING";
}
}
int main(){
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 hui::gfx::Renderer renderer;if(!renderer.init())hui::sys::park();
 Font font;std::string fontBytes=load("/app0/assets/fonts/inter-semibold.huifont");
 if(fontBytes.empty())fontBytes=load("/data/homebrew/PPSA99684/assets/fonts/inter-semibold.huifont");
 if(!font.load(fontBytes))hui::sys::park();
 unsigned atlas=renderer.batch().create_font_texture(font);
 hui::ps5::Pad pad;pad.open();
 torrent::DownloadManager manager;
 int page=0,librarySelected=0,downloadSelected=0,settingSelected=0;
 unsigned previous=0;
 std::string notice="Ready. Add torrents from Library or enable Auto Download.";
 auto library=torrent::discoverTorrents();
 const char* tabs[]={"Dashboard","Torrent Library","Downloads","Settings","Logs","About"};
 const char* sub[]={"Queue overview","All available torrents","All download jobs","Automatic downloads","Live diagnostics","Engine information"};
 for(;;){
  manager.tick();
  auto jobs=manager.jobs();
  auto settings=manager.settings();
  int active=0,queued=0,finished=0,errors=0;
  uint64_t received=0,total=0;double totalSpeed=0;
  for(const auto& j:jobs){
   if(j.running)active++;
   if(j.queued)queued++;
   if(j.completedJob)finished++;
   if(j.failed)errors++;
   received+=j.completed;total+=j.total;totalSpeed+=j.speed;
  }
  std::array<hui::PadSample,64> samples{};unsigned pressed=0;
  size_t n=pad.read(samples);
  for(size_t i=0;i<n;i++){pressed|=samples[i].buttons&~previous;previous=samples[i].buttons;}
  if(pressed&hui::pad_bits::kL1)page=(page+5)%6;
  if(pressed&hui::pad_bits::kR1)page=(page+1)%6;
  if(pressed&hui::pad_bits::kUp){
   if(page==1&&librarySelected>0)librarySelected--;
   else if(page==2&&downloadSelected>0)downloadSelected--;
   else if(page==3&&settingSelected>0)settingSelected--;
  }
  if(pressed&hui::pad_bits::kDown){
   if(page==1&&librarySelected+1<(int)library.size())librarySelected++;
   else if(page==2&&downloadSelected+1<(int)jobs.size())downloadSelected++;
   else if(page==3&&settingSelected<4)settingSelected++;
  }
  if(pressed&hui::pad_bits::kCircle){
   if(page!=0)page=0;
   else{
    manager.stopAll();pad.close();renderer.release();display.close();
    hui::sys::quit();
   }
  }
  if(pressed&hui::pad_bits::kTriangle){
   if(page==1){
    library=torrent::discoverTorrents();manager.scan(true);
    librarySelected=std::max(0,std::min(librarySelected,(int)library.size()-1));
    notice="Torrent library refreshed";
   }else if(page==2){
    notice="Queue refreshed";
   }
  }
  if(pressed&hui::pad_bits::kSquare){
   if(page==1){
    int count=manager.enqueueAll(library);
    notice=std::to_string(count)+" torrents added. Open Downloads.";
   }else if(page==2&&!jobs.empty()){
    manager.cancel((size_t)downloadSelected);
    notice="Cancel requested for selected download";
   }else if(page==0){
    for(size_t i=0;i<jobs.size();i++)if(jobs[i].running)manager.cancel(i);
    notice="Stopping active downloads";
   }
  }
  if(pressed&hui::pad_bits::kCross){
   if(page==0){
    if(library.empty())notice="No torrent files available";
    else{std::string err;if(manager.enqueue(library[0],&err))notice="Ubuntu Desktop queued";else notice=err;}
   }else if(page==1&&!library.empty()){
    std::string err;
    if(manager.enqueue(library[(size_t)librarySelected],&err))
     notice="Added to Downloads: "+torrent::testDisplayName(library[(size_t)librarySelected]);
    else notice=err;
   }else if(page==2&&!jobs.empty()){
    manager.retry((size_t)downloadSelected);
    notice="Retry requested (failed/cancelled jobs only)";
   }else if(page==3){
    if(settingSelected==0)manager.setAutoDownload(!settings.autoDownload);
    if(settingSelected==1)manager.setMaxActive(settings.maxActive>=3?1:settings.maxActive+1);
    if(settingSelected==2)manager.setDeleteCompleted(!settings.deleteCompletedTorrents);
    if(settingSelected==3){
     if(!manager.setUsbStorage(!settings.usb))notice="Stop active downloads and mount USB0 first";
    }
    if(settingSelected==4)manager.setScanSeconds(settings.scanSeconds>=10?2:settings.scanSeconds+2);
    notice="Settings saved to /data/ps5-torrent/settings.ini";
   }
  }
  // Refresh after the controller changed the job queue or settings.
  jobs=manager.jobs();settings=manager.settings();
  active=queued=finished=errors=0;received=total=0;totalSpeed=0;
  for(const auto& j:jobs){
   active+=j.running;queued+=j.queued;finished+=j.completedJob;errors+=j.failed;
   received+=j.completed;total+=j.total;totalSpeed+=j.speed;
  }
  DrawList d;
  d.gradient_rect({0,0,1920,1080},0,Color::rgb(0x081624),Color::rgb(0x050e19));
  for(int i=0;i<8;i++)d.arc(1580,320,280+i*33,2,3.5f,6.25f,Color::rgb(0x23d8b0,.07f));
  d.rounded_rect({0,0,1920,102},0,Color::rgb(0x0d1b2d));
  d.circle(79,51,28,mint);d.circle(79,51,18,Color::rgb(0x0d1b2d));
  d.line(63,51,95,51,4,mint);d.line(79,35,79,67,4,mint);
  text(d,font,atlas,"TORRENT FLOW",127,55,35);
  text(d,font,atlas,"by YAHBALLON",462,55,21,mint);
  text(d,font,atlas,"v0.3.2",1830,56,23,white,Align::right);
  card(d,27,125,335,872);
  for(int i=0;i<6;i++){
   float y=144+i*106;card(d,43,y,302,91,page==i);
   text(d,font,atlas,tabs[i],65,y+40,23,page==i?mint:white);
   text(d,font,atlas,sub[i],65,y+72,17,muted);
  }
  text(d,font,atlas,"QUEUE MANAGER",60,813,19,mint);
  text(d,font,atlas,std::to_string(active)+" active  |  "+std::to_string(queued)+" queued",60,852,19,white);
  text(d,font,atlas,std::to_string(finished)+" complete  |  "+std::to_string(errors)+" errors",60,885,19,muted);
  text(d,font,atlas,settings.autoDownload?"AUTO DOWNLOAD ON":"AUTO DOWNLOAD OFF",60,942,19,settings.autoDownload?mint:muted);
  if(page==0){
   card(d,386,125,1496,246);
   text(d,font,atlas,"DOWNLOAD QUEUE",426,188,23,mint);
   text(d,font,atlas,std::to_string(jobs.size())+" torrents tracked",426,258,43);
   text(d,font,atlas,"Aggregate verified data",1150,222,22,muted);
   text(d,font,atlas,size(received)+" / "+size(total),1150,261,27,white);
   bar(d,425,325,1415,total?(float)((double)received/total):0,mint);
   const char* labels[]={"TOTAL DOWNLOAD RATE","ACTIVE","QUEUED","COMPLETED"};
   std::string vals[]={rate(totalSpeed),std::to_string(active),std::to_string(queued),std::to_string(finished)};
   for(int i=0;i<4;i++){
    float x=387+378*i;card(d,x,389,362,154);
    text(d,font,atlas,labels[i],x+22,431,18,muted);
    text(d,font,atlas,vals[i],x+22,492,34,i==0?mint:white);
   }
   card(d,386,560,988,419);card(d,1390,560,492,419);
   text(d,font,atlas,"RECENT DOWNLOADS",416,613,23,mint);
   const size_t count=std::min<size_t>(5,jobs.size());
   for(size_t i=0;i<count;i++){
    const auto& j=jobs[jobs.size()-1-i];float y=667+i*57;
    text(d,font,atlas,shorten(j.name,45),418,y,21);
    text(d,font,atlas,phase(j)+"  "+(j.total?std::to_string((int)(100.0*j.completed/j.total))+"%":"--"),
        1322,y,19,j.completedJob?mint:muted,Align::right);
   }
   text(d,font,atlas,"QUICK ACTIONS",1420,612,22,mint);
   text(d,font,atlas,"R1  Library / Downloads",1420,684,21);
   text(d,font,atlas,"X   Queue Ubuntu test",1420,742,21);
   text(d,font,atlas,"SQUARE  Stop all active",1420,800,21);
   text(d,font,atlas,"Auto: "+std::string(settings.autoDownload?"Enabled":"Disabled"),1420,870,22,settings.autoDownload?mint:gold);
   text(d,font,atlas,"Settings saved automatically",1420,923,17,muted);
  }else if(page==1||page==2){
   const bool downloads=page==2;
   card(d,386,125,1496,865);
   text(d,font,atlas,downloads?"ALL DOWNLOADS":"TORRENT LIBRARY",423,198,41);
   text(d,font,atlas,downloads?"X Retry failed  |  SQUARE Cancel selected  |  D-Pad Select":
        "X Queue selected  |  SQUARE Download All  |  TRIANGLE Refresh",424,253,21,muted);
   const size_t count=downloads?jobs.size():library.size();
   const int selected=downloads?downloadSelected:librarySelected;
   const size_t start=(size_t)selected>4?(size_t)selected-4:0;
   if(!count)text(d,font,atlas,downloads?"No downloads yet. Add torrents from Library.":
        "No torrents available in library",434,381,24,gold);
   for(size_t i=start;i<count&&i<start+6;i++){
    float y=299+(i-start)*107;card(d,413,y,1440,98,(int)i==selected);
    if(downloads){
     const auto& j=jobs[i];
     text(d,font,atlas,shorten(j.name,61),441,y+35,24,(int)i==selected?mint:white);
     text(d,font,atlas,phase(j)+"  |  "+shorten(j.status,68),443,y+72,17,muted);
     float p=j.total?(float)((double)j.completed/j.total):0;
     bar(d,1230,y+23,350,p,j.failed?gold:mint);
     text(d,font,atlas,std::to_string((int)(p*100))+"%   "+rate(j.speed),1818,y+77,18,j.completedJob?mint:muted,Align::right);
    }else{
     text(d,font,atlas,shorten(torrent::testDisplayName(library[i]),72),443,y+37,24,(int)i==selected?mint:white);
     text(d,font,atlas,shorten(library[i],120),444,y+74,17,muted);
     text(d,font,atlas,torrent::testSizeHint(library[i]),1817,y+39,19,gold,Align::right);
    }
   }
   text(d,font,atlas,std::to_string(count)+" entries  |  "+(count?std::to_string(start+1):"0")+
        "-"+std::to_string(std::min(start+6,count)),1810,966,18,muted,Align::right);
  }else if(page==3){
   card(d,386,125,1496,865);
   text(d,font,atlas,"SETTINGS",424,202,42);
   text(d,font,atlas,"Changes are saved to "+manager.iniPath(),425,250,20,muted);
   const char* keys[]={"Auto Download","Simultaneous Downloads","Delete Completed .torrent","Download Storage","Auto Scan Interval"};
   std::string values[]={settings.autoDownload?"ON":"OFF",std::to_string(settings.maxActive),
       settings.deleteCompletedTorrents?"ON":"OFF",settings.usb?"USB0":"Internal",
       std::to_string(settings.scanSeconds)+" seconds"};
   for(int i=0;i<5;i++){
    float y=286+i*128;card(d,411,y,1442,111,settingSelected==i);
    text(d,font,atlas,keys[i],443,y+49,28,settingSelected==i?mint:white);
    text(d,font,atlas,values[i],1815,y+49,27,mint,Align::right);
    text(d,font,atlas,i==0?"Watch /data/ps5-torrent/torrents for new metadata":
     i==1?"Up to 3 torrents at the same time":
     i==2?"Remove metadata only after SHA-1 verified completion":
     i==3?"Choose internal disk or USB0 when no transfers are active":
     "How often to look for new .torrent files",444,y+82,18,muted);
   }
   text(d,font,atlas,"X  Change selected setting  |  D-Pad  Navigate",432,962,20,muted);
  }else if(page==4){
   card(d,386,125,1496,866);
   text(d,font,atlas,"LIVE TRANSFER LOGS",429,209,42);
   text(d,font,atlas,"Log file: "+shorten(torrent::logPath(),90),430,268,22,mint);
   auto lines=torrent::recentLogs();size_t count=std::min<size_t>(12,lines.size());
   for(size_t i=0;i<count;i++){
    const auto& line=lines[lines.size()-count+i];float y=337+i*49;
    if(i%2==0)d.rounded_rect({420,y-28,1414,44},7,Color::rgb(0x11263a));
    const bool fail=line.find("FAILED")!=std::string::npos||line.find("timeout")!=std::string::npos;
    text(d,font,atlas,shorten(line,120),433,y,18,fail?gold:muted);
   }
  }else{
   card(d,386,125,1496,866);
   text(d,font,atlas,"ABOUT TORRENT FLOW",429,213,43);
   const char* lines[]={
    "v0.3.2 / PPSA99684  |  BitTorrent v1 / Multi-file support",
    "Multi-download queue with independent cancel per torrent",
    "Persistent queue stored in /data/ps5-torrent/cache/queue.db",
    "Automatic folder: /data/ps5-torrent/torrents",
    "INI settings: /data/ps5-torrent/settings.ini",
    "Safe cleanup: user-supplied .torrent deleted only after completion",
    "HTTP / HTTPS / UDP trackers, per-piece SHA-1 integrity",
    "No DHT, magnets, v2-only torrents or seeding yet"
   };
   for(int i=0;i<8;i++)text(d,font,atlas,lines[i],431,299+i*78,23,i==7?gold:muted);
  }
  d.rounded_rect({25,1007,1870,53},12,Color::rgb(0x102237));
  text(d,font,atlas,"L1/R1 Tabs     D-Pad Select     X Action     SQUARE Cancel / Download All     O Back",45,1041,18,muted);
  text(d,font,atlas,shorten(notice,49),1849,1041,18,mint,Align::right);
  renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());
  display.swap();hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
