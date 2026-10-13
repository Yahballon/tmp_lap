// SPDX-License-Identifier: GPL-3.0-or-later
// Native PS5 Speedtest dashboard v0.2.0. All displayed measurements are real.
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "speedtest.hpp"
#include <array>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <vector>
#include <sys/stat.h>
using namespace hui::gfx;
static const Color white=Color::rgb(0xeaf4ff),muted=Color::rgb(0x9db7dc),cyan=Color::rgb(0x03d8ff),blue=Color::rgb(0x0084ff),purple=Color::rgb(0xcd40ff),green=Color::rgb(0x45df81),edge=Color::rgb(0x174a91),dark=Color::rgb(0x04112c);
static std::string load(const char* p){FILE* f=fopen(p,"rb");if(!f)return {};fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<1||n>16000000){fclose(f);return {};}std::string s((size_t)n,0);fread(s.data(),1,s.size(),f);fclose(f);return s;}
static void t(DrawList& u,const Font& f,unsigned a,const std::string& v,float x,float y,float z,Color c=white,Align al=Align::left){u.text(f,a,v,x,y,z,c,al);}
static std::string fmt(double n,int decimals=1){char b[48];std::snprintf(b,sizeof(b),decimals?"%.1f":"%.0f",n);return b;}
static void panel(DrawList& u,float x,float y,float w,float h,bool focus=false){u.shadow({x,y,w,h},18,15,Color::rgb(0x0044bb,.15f));u.bordered_rect({x,y,w,h},16,Color::rgb(focus?0x082d67:0x04112a),focus?3:1,focus?cyan:edge);}
static void divider(DrawList& u,float x,float y,float w){u.line(x,y,x+w,y,1,Color::rgb(0x123d70));}
static void pill(DrawList& u,const Font& f,unsigned a,const std::string& s,float x,float y,float w,bool sel){u.bordered_rect({x,y,w,50},10,Color::rgb(sel?0x006de0:0x081a38),sel?2:1,sel?cyan:edge);t(u,f,a,s,x+w/2,y+33,19,sel?white:muted,Align::center);}
static void plot(DrawList& ui,const std::vector<double>& v,float x,float y,float w,float h,Color col,double maximum){
 if(v.empty()||maximum<=0)return;
 float lastX=x,lastY=y+h;
 for(size_t i=0;i<v.size();i++){float xx=x+(i+1)*(w/(float)v.size());float yy=y+h-(float)std::min(1.0,v[i]/maximum)*h;
 ui.line(lastX,lastY,xx,yy,3,col);ui.circle(xx,yy,4,col);lastX=xx;lastY=yy;}
}

static void flag(DrawList& ui,const Font& font,unsigned atlas,const std::string& code,float x,float y){
 auto rect=[&](float xx,float yy,float w,float h,unsigned c){ui.rounded_rect({xx,yy,w,h},0,Color::rgb(c));};
 rect(x,y,48,30,0xf1f3f8);
 auto horizontal=[&](unsigned a,unsigned b,unsigned c){rect(x,y,48,10,a);rect(x,y+10,48,10,b);rect(x,y+20,48,10,c);};
 if(code=="DE")horizontal(0x161616,0xdd2525,0xf2cd32);
 else if(code=="NL")horizontal(0xbd2433,0xffffff,0x204eab);
 else if(code=="IT"){rect(x,y,16,30,0x159447);rect(x+32,y,16,30,0xd6333b);}
 else if(code=="GB"){rect(x,y,48,30,0x23448b);ui.line(x,y,x+48,y+30,6,white);ui.line(x+48,y,x,y+30,6,white);rect(x+19,y,10,30,0xd62838);rect(x,y+10,48,10,0xd62838);}
 else if(code=="US"){for(int i=0;i<7;i++)rect(x,y+i*4.3f,48,2.4f,0xc62b3a);rect(x,y,23,17,0x24458d);for(int k=0;k<5;k++)ui.circle(x+4+(k%3)*7,y+4+(k/3)*7,1.3f,white);}
 else if(code=="JP"){ui.circle(x+24,y+15,9,Color::rgb(0xc72e41));}
 else if(code=="SG"){horizontal(0xe23e43,0xe23e43,0xffffff);ui.circle(x+12,y+7,5,white);ui.circle(x+15,y+5,4,Color::rgb(0xe23e43));}
 else if(code=="IN")horizontal(0xef9235,0xffffff,0x249451);
 else if(code=="ZA"){rect(x,y,48,15,0xd53937);rect(x,y+15,48,15,0x14498b);ui.line(x,y+15,x+48,y+15,8,white);ui.line(x,y+15,x+48,y+15,5,Color::rgb(0x249a4b));}
 else if(code=="CZ"){horizontal(0xffffff,0xffffff,0xd43645);ui.line(x,y+1,x+24,y+15,4,Color::rgb(0x2354a0));ui.line(x,y+29,x+24,y+15,4,Color::rgb(0x2354a0));}
 else {rect(x,y,48,30,0x12417a);t(ui,font,atlas,code,x+24,y+22,15,white,Align::center);}
 ui.bordered_rect({x,y,48,30},1,Color::rgb(0x04152c,0.01f),1,Color::rgb(0x426ba3));
}
struct Result {double d=0,u=0,p=0,j=0;long when=0;};
static const char* historyPath="/data/homebrew/PPSA99573/speedtest-history.csv";
static std::vector<Result> loadHistory(){
 std::vector<Result> out;FILE* f=fopen(historyPath,"rb");if(!f)return out;
 Result r;while(out.size()<50 && fscanf(f,"%lf,%lf,%lf,%lf,%ld\n",&r.d,&r.u,&r.p,&r.j,&r.when)==5){if(r.d>=0&&r.u>=0&&r.p>=0&&r.when>0)out.push_back(r);}
 fclose(f);return out;
}
static bool saveHistory(const std::vector<Result>& rows) {
 mkdir("/data/homebrew/PPSA99573",0755);
 std::string path=std::string(historyPath)+".tmp";
 FILE* f=fopen(path.c_str(),"wb");if(!f)return false;
 for(const auto& r:rows)if(fprintf(f,"%.5f,%.5f,%.5f,%.5f,%ld\n",r.d,r.u,r.p,r.j,r.when)<0){fclose(f);remove(path.c_str());return false;}
 if(fclose(f)!=0){remove(path.c_str());return false;}
 if(rename(path.c_str(),historyPath)!=0){remove(path.c_str());return false;}
 return true;
}
int main(){
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 hui::gfx::Renderer renderer;if(!renderer.init())hui::sys::park();
 Font font;std::string bytes=load("/app0/assets/fonts/inter-semibold.huifont");
 if(bytes.empty())bytes=load("/data/homebrew/PPSA99573/assets/fonts/inter-semibold.huifont");
 if(!font.load(bytes))hui::sys::park();
 unsigned atlas=renderer.batch().create_font_texture(font);
 hui::ps5::Pad pad;pad.open();unsigned previous=0;
 const char* menus[]={"Speed Test","Server Select","Results","Network Info","Settings","About"};
 const char* subs[]={"Test your connection","Choose test server","View test history","Show connection details","Adjust options","App information"};
 int menu=0,tab=0,historyScroll=0,graphTab=0,serverScroll=0,serverFocus=0;
 bool serverListFocus=false;
 bool measured=false,previouslyRunning=false,autoSave=true;
 std::vector<Result> history=loadHistory();
 int frame=0;
 for(;;){
  std::array<hui::PadSample,64> samples{};unsigned pressed=0;size_t n=pad.read(samples);
  for(size_t i=0;i<n;i++){pressed|=samples[i].buttons&~previous;previous=samples[i].buttons;}
  if(pressed&hui::pad_bits::kUp){if(menu==1&&serverListFocus){serverFocus=std::max(0,serverFocus-1);if(serverFocus<serverScroll)serverScroll=serverFocus;}else if(menu==2&&historyScroll>0)historyScroll--;else menu=(menu+5)%6;}
  if(pressed&hui::pad_bits::kDown){if(menu==1&&serverListFocus){serverFocus=std::min(speedtest::serverCount()-1,serverFocus+1);if(serverFocus>=serverScroll+9)serverScroll=serverFocus-8;}else if(menu==2&&historyScroll+6<(int)history.size())historyScroll++;else menu=(menu+1)%6;}
  if(pressed&hui::pad_bits::kLeft)tab=(tab+3)%4;
  if(pressed&hui::pad_bits::kRight)tab=(tab+1)%4;
  if(pressed&hui::pad_bits::kL1)graphTab=(graphTab+3)%4;
  if(pressed&hui::pad_bits::kR1)graphTab=(graphTab+1)%4;
  if(pressed&hui::pad_bits::kCircle){if(menu==1&&serverListFocus)serverListFocus=false;else if(menu!=0)menu=0;else {pad.close();renderer.release();display.close();hui::sys::quit();}}
  if(pressed&hui::pad_bits::kTriangle){menu=0;graphTab=0;serverListFocus=false;}
  if(pressed&hui::pad_bits::kSquare){menu=1;serverListFocus=true;serverFocus=speedtest::selectedServer();serverScroll=std::max(0,serverFocus-7);}
  if(pressed&hui::pad_bits::kCross){
   if(menu==0)speedtest::start();else if(menu==1){if(!serverListFocus){serverListFocus=true;serverFocus=speedtest::selectedServer();}else if(speedtest::selectServer(serverFocus)){serverListFocus=false;menu=0;speedtest::start();}}
   else if(menu==4)autoSave=!autoSave;
  }
  auto s=speedtest::snapshot();
  if(previouslyRunning&&!s.running&&s.completed&&!measured){
   measured=true;
   if(autoSave){Result r;r.d=s.down;r.u=s.up;r.p=s.ping;r.j=s.jitter;r.when=(long)std::time(nullptr);history.insert(history.begin(),r);if(history.size()>50)history.resize(50);saveHistory(history);}
  }
  if(s.running){previouslyRunning=true;measured=false;}
  DrawList ui;
  ui.gradient_rect({0,0,1920,1080},0,Color::rgb(0x020819),Color::rgb(0x021640));
  // Original procedural network globe - no commercial console branding.
  for(int j=0;j<9;j++){float rad=200+j*26;ui.arc(1320,130,rad,2,3.18f,6.15f,Color::rgb(0x0476ff,.15f));}
  for(int j=0;j<11;j++)ui.line(850+j*86,5,950+j*56,146,1,Color::rgb(0x1578f3,.10f));
  ui.circle(108,60,40,Color::rgb(0x073a91));ui.arc(108,60,37,5,0,6.28f,cyan);
  ui.line(70,60,146,60,3,cyan);ui.line(108,24,108,96,3,cyan);
  t(ui,font,atlas,"WWW SPEEDTEST",196,69,52);
  t(ui,font,atlas,"Test your internet speed on PS5",198,108,23,muted);
  t(ui,font,atlas,"by YAHBALLON",1580,55,18,cyan);
  t(ui,font,atlas,"v0.4.1",1835,55,20,white,Align::right);
  panel(ui,24,161,380,848);
  const char* symbols[]={"[>]","[=]","[|]","[@]","[*]","[i]"};
  for(int i=0;i<6;i++){
    float y=179+i*106;
    if(i==menu){ui.shadow({35,y,357,87},14,15,Color::rgb(0x036eff,.45f));ui.bordered_rect({35,y,357,87},12,Color::rgb(0x036fe8),3,cyan);}
    t(ui,font,atlas,symbols[i],62,y+48,27,i==menu?white:muted);
    t(ui,font,atlas,menus[i],144,y+40,25);
    t(ui,font,atlas,subs[i],144,y+68,17,muted);
    if(i<5)divider(ui,50,y+100,336);
  }
  // Always keep dashboard visible on home, page-specific details on other tabs.
  if(menu==0){
   panel(ui,415,144,767,570);
   const float cx=800,cy=430;
   ui.arc(cx,cy,254,18,3.53f,5.95f,cyan);
   ui.arc(cx,cy,254,18,5.95f,8.94f,purple);
   ui.arc(cx,cy,238,3,3.53f,8.94f,Color::rgb(0x2465ff));
   for(int i=0;i<=16;i++){
    float angle=3.53f+(8.94f-3.53f)*i/16.f;
    float x1=cx+226*std::cos(angle),y1=cy+226*std::sin(angle);
    float x2=cx+242*std::cos(angle),y2=cy+242*std::sin(angle);
    ui.line(x1,y1,x2,y2,(i%4==0)?4:2,muted);
   }
   // Needle sweeps with each sampled throughput value.
   const double value=s.running?s.liveMbps:(s.completed?s.down:0);
   const double fraction=std::min(1.0,value/1000.0);
   float needleAngle=3.53f+(8.94f-3.53f)*(float)fraction;
   ui.line(cx,cy,cx+198*std::cos(needleAngle),cy+198*std::sin(needleAngle),7,cyan);
   ui.circle(cx,cy,139,Color::rgb(0x086fe1));ui.circle(cx,cy,132,dark);
   if(s.running||s.completed)t(ui,font,atlas,fmt(value)+" Mbps",cx,cy+85,24,cyan,Align::center);
   t(ui,font,atlas,s.running?"TESTING":s.completed?"RESTART":"START",cx,cy-6,42,white,Align::center);
   t(ui,font,atlas,s.running?"PLEASE WAIT":"SPEED TEST",cx,cy+36,20,muted,Align::center);
   t(ui,font,atlas,"0",577,563,19);t(ui,font,atlas,"1000",1017,563,19);
   panel(ui,435,595,727,101);
   const char* labels[]={"Download","Upload","Ping"};
   for(int i=0;i<3;i++){float x=468+i*228;double v=i==0?s.down:i==1?s.up:s.ping;
    t(ui,font,atlas,labels[i],x,630,22,i==0?cyan:i==1?purple:green);
    t(ui,font,atlas,(s.running||s.completed)?fmt(v,i==2?0:1):"--",x,669,34);
    t(ui,font,atlas,i==2?"ms":"Mbps",x+111,671,17,muted);
    if(i<2)ui.line(x+202,615,x+202,677,1,edge);
   }
   panel(ui,1195,144,696,188);
   t(ui,font,atlas,"Test Server",1260,187,25);
   t(ui,font,atlas,speedtest::servers()[speedtest::selectedServer()].name,1270,245,25);
   t(ui,font,atlas,"Verified HTTPS   |   X on Server Select to change",1270,288,17,muted);
   pill(ui,font,atlas,"SQUARE",1690,218,172,true);
   panel(ui,1195,340,696,374);
   const char* tabs[]={"Speed Graph","Download","Upload","Ping"};
   for(int i=0;i<4;i++)pill(ui,font,atlas,tabs[i],1212+i*167,356,155,graphTab==i);
   for(int i=0;i<5;i++){float y=446+i*48;ui.line(1250,y,1864,y,1,edge);}
   double scale=std::max(100.0,std::max(s.down,s.up)*1.20);
   for(double v:s.downGraph)scale=std::max(scale,v*1.1);
   for(double v:s.upGraph)scale=std::max(scale,v*1.1);
   if(graphTab==0||graphTab==1)plot(ui,s.downGraph,1251,446,606,188,cyan,scale);
   if(graphTab==0||graphTab==2)plot(ui,s.upGraph,1251,446,606,188,purple,scale);
   if(graphTab==3)t(ui,font,atlas,"Latency: "+fmt(s.ping)+" ms  |  Jitter: "+fmt(s.jitter)+" ms",1250,555,21,green);
   t(ui,font,atlas,graphTab==0?"Download (cyan)       Upload (purple)":tabs[graphTab],1258,678,20,muted);
   panel(ui,415,727,460,283);panel(ui,890,727,455,283);panel(ui,1360,727,531,283);
   t(ui,font,atlas,"Connection Information",440,765,23);divider(ui,436,779,412);
   t(ui,font,atlas,"Endpoint",438,816,20,muted);t(ui,font,atlas,speedtest::servers()[speedtest::selectedServer()].host,608,816,16);
   t(ui,font,atlas,"Protocol",438,852,20,muted);t(ui,font,atlas,"Verified HTTPS",608,852,19);
   t(ui,font,atlas,"Local IP",438,888,20,muted);t(ui,font,atlas,s.localIp,608,888,18);
   t(ui,font,atlas,"Latency / Jitter",438,924,19,muted);t(ui,font,atlas,fmt(s.ping)+" / "+fmt(s.jitter)+" ms",608,924,18);
   t(ui,font,atlas,"Quick Servers",915,765,23);divider(ui,910,779,413);
   t(ui,font,atlas,speedtest::servers()[speedtest::selectedServer()].name,918,825,21,cyan);
   t(ui,font,atlas,"SQUARE to choose servers",918,862,19,muted);
   t(ui,font,atlas,"LibreSpeed server directory",918,892,19,muted);
   t(ui,font,atlas,"Recent Results",1385,765,23);divider(ui,1380,779,491);
   if(history.empty())t(ui,font,atlas,"Run a test to populate history",1390,824,20,muted);
   for(size_t i=0;i<history.size()&&i<5;i++){const auto& r=history[i];float y=816+i*35;
     t(ui,font,atlas,fmt(r.d),1390,y,19,cyan);t(ui,font,atlas,fmt(r.u),1513,y,19,purple);
     t(ui,font,atlas,fmt(r.p)+" ms",1635,y,19,green);
     char date[32];std::tm* tm=std::localtime(&r.when);
     if(tm){std::strftime(date,sizeof(date),"%d.%m %H:%M",tm);t(ui,font,atlas,date,1865,y,17,muted,Align::right);}
   }
  }else{
   panel(ui,415,144,1476,866);
   t(ui,font,atlas,menus[menu],470,231,50);
   divider(ui,465,252,1374);
   if(menu==1){
    t(ui,font,atlas,serverListFocus?"Server list focus: X select, O return to sidebar":"Press X to browse servers; D-Pad navigates sidebar",470,319,26);
    const auto* hosts=speedtest::servers();
    for(int i=serverScroll;i<speedtest::serverCount()&&i<serverScroll+9;i++){
      const float y=360+(i-serverScroll)*67;
      ui.bordered_rect({470,y-33,1320,59},10,Color::rgb(i==serverFocus&&serverListFocus?0x0b4383:0x0a1a36),i==serverFocus&&serverListFocus?2:1,i==serverFocus&&serverListFocus?cyan:edge);
      flag(ui,font,atlas,hosts[i].flag,488,y-16);
      t(ui,font,atlas,hosts[i].name,558,y+9,23,i==serverFocus&&serverListFocus?white:muted);
      if(i==speedtest::selectedServer())t(ui,font,atlas,"SELECTED",1740,y+9,18,green,Align::right);
    }
    t(ui,font,atlas,"Hosted servers may be unavailable. No location or speed is fabricated.",470,940,19,muted);
    t(ui,font,atlas,s.status,470,984,18,muted);
   }
   if(menu==2){
    t(ui,font,atlas,"Saved results: "+std::to_string(history.size()),470,323,26,muted);
    if(history.empty())t(ui,font,atlas,"No saved tests yet. Start a test from the Speed Test menu.",470,395,24,muted);
    for(int i=historyScroll;i<(int)history.size()&&i<historyScroll+11;i++){const Result& r=history[i];float y=385+(i-historyScroll)*51;
     if(i%2==0)ui.rounded_rect({472,y-28,1310,47},7,Color::rgb(0x092753));
     t(ui,font,atlas,fmt(r.d)+" Mbps down",500,y,22,cyan);
     t(ui,font,atlas,fmt(r.u)+" Mbps up",820,y,22,purple);
     t(ui,font,atlas,fmt(r.p)+" ms ping",1150,y,22,green);
     char date[32];std::tm* tm=std::localtime(&r.when);if(tm){std::strftime(date,sizeof(date),"%d.%m.%Y %H:%M",tm);t(ui,font,atlas,date,1755,y,20,muted,Align::right);}
    }
   }
   if(menu==3){
    t(ui,font,atlas,std::string("Server: ")+speedtest::servers()[speedtest::selectedServer()].host,475,348,29);
    t(ui,font,atlas,"Transport: HTTPS / TLS with certificate validation",475,406,25);
    t(ui,font,atlas,"Latency includes TLS and connection setup time.",475,463,25,muted);
    t(ui,font,atlas,"Local PS5 IP: "+s.localIp,475,533,25,cyan);
    t(ui,font,atlas,"Remote server IP: "+s.remoteIp,475,590,25,cyan);
    t(ui,font,atlas,"These are socket addresses; public IP and NAT type are not detected.",475,650,22,muted);
    t(ui,font,atlas,"Last test: "+s.status,475,720,19,muted);
   }
   if(menu==4){
    t(ui,font,atlas,"Auto-save completed tests",475,355,28);
    pill(ui,font,atlas,autoSave?"ON":"OFF",1030,319,170,true);
    t(ui,font,atlas,"Press X to toggle. Up to 50 results are stored locally.",475,411,22,muted);
    t(ui,font,atlas,std::string("Measurement host: ")+speedtest::servers()[speedtest::selectedServer()].host,475,484,23);
   }
   if(menu==5){
    t(ui,font,atlas,"WWW SPEEDTEST by YAHBALLON",475,361,32);
    t(ui,font,atlas,"v0.3.2 | PPSA99573 | Native C++ / OpenGL",475,426,25,muted);
    t(ui,font,atlas,"Real HTTPS measurements. No simulated network data.",475,497,24,cyan);
   }
  }
  if(s.running){ui.rounded_rect({434,1015,1450*(float)s.progress,7},3,cyan);}
  t(ui,font,atlas,"X Select / Start    O Sidebar    D-Pad Navigate    SQUARE Servers    L1/R1 Tabs",47,1054,20,muted);
  if(s.running||(!s.completed&&s.status.find("failed")!=std::string::npos))t(ui,font,atlas,s.status,1860,1052,17,s.running?cyan:purple,Align::right);
  renderer.begin();renderer.draw(ui);renderer.present(0,display.width(),display.height());display.swap();hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
