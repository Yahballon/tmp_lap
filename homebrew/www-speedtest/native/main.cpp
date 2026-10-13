// SPDX-License-Identifier: GPL-3.0-or-later
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "speedtest.hpp"
#include <array>
#include <cstdio>
#include <string>
using namespace hui::gfx;
static std::string readFile(const char* path) { FILE* f=fopen(path,"rb");if(!f)return {};fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<1||n>16000000){fclose(f);return {};}std::string s((size_t)n,0);fread(s.data(),1,s.size(),f);fclose(f);return s;}
static const Color white=Color::rgb(0xeff8ff),muted=Color::rgb(0xa6c4e4),cyan=Color::rgb(0x19d8ff),blue=Color::rgb(0x177aff),purple=Color::rgb(0xce49ff),green=Color::rgb(0x38db83);
static void txt(DrawList& u,const Font& f,unsigned a,const std::string& s,float x,float y,float size,Color c=white,Align align=Align::left){u.text(f,a,s,x,y,size,c,align);}
static void box(DrawList& u,float x,float y,float w,float h,bool active=false){u.bordered_rect({x,y,w,h},18,Color::rgb(active?0x103f83:0x061530),active?3:1,active?cyan:Color::rgb(0x164b91));}
static std::string fmt(double n,int digits=1){char b[60];snprintf(b,sizeof(b),digits==0?"%.0f":"%.1f",n);return b;}
static void graph(DrawList& ui,const std::vector<double>& v,Rect r,Color c,double scale){
 if(v.empty())return;
 float px=r.x,py=r.y+r.h;
 for(size_t i=0;i<v.size();++i){float x=r.x+r.w*(float)(i+1)/(float)v.size();float y=r.y+r.h-(float)(v[i]/scale)*r.h;if(y<r.y)y=r.y;if(i>0)ui.line(px,py,x,y,4,c);else ui.line(r.x,r.y+r.h,x,y,4,c);ui.circle(x,y,5,c);px=x;py=y;}
}
int main(){
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 hui::gfx::Renderer renderer;if(!renderer.init())hui::sys::park();
 Font font;
 std::string bytes=readFile("/app0/assets/fonts/inter-semibold.huifont");
 if(bytes.empty())bytes=readFile("/data/homebrew/PPSA99573/assets/fonts/inter-semibold.huifont");
 if(!font.load(bytes))hui::sys::park();
 unsigned atlas=renderer.batch().create_font_texture(font);
 hui::ps5::Pad pad;pad.open();unsigned previous=0;int selected=0;
 const char* menu[]={"Speed Test","Results","Network Info","About"};
 for(;;){
  std::array<hui::PadSample,64> samples{};unsigned pressed=0;size_t n=pad.read(samples);
  for(size_t i=0;i<n;i++){pressed|=samples[i].buttons&~previous;previous=samples[i].buttons;}
  if(pressed&hui::pad_bits::kUp)selected=(selected+3)%4;
  if(pressed&hui::pad_bits::kDown)selected=(selected+1)%4;
  if(pressed&hui::pad_bits::kCircle){if(selected)selected=0;else{pad.close();renderer.release();display.close();hui::sys::quit();}}
  if(pressed&hui::pad_bits::kTriangle)selected=0;
  if((pressed&hui::pad_bits::kCross)&&selected==0)speedtest::start();
  auto s=speedtest::snapshot();
  DrawList ui;
  ui.gradient_rect({0,0,1920,1080},0,Color::rgb(0x020715),Color::rgb(0x071e53));
  for(int i=0;i<13;i++)ui.arc(1460,110,360+i*14,2,3.05f,6.19f,Color::rgb(0x0665e5,.10f));
  txt(ui,font,atlas,"WWW SPEEDTEST",190,81,54);
  txt(ui,font,atlas,"by YAHBALLON  |  Test your internet speed on PS5",196,122,22,muted);
  txt(ui,font,atlas,"v0.1.0",1835,76,21,white,Align::right);
  box(ui,25,165,365,810);
  for(int i=0;i<4;i++){if(i==selected)ui.bordered_rect({42.0f,195.0f+i*101,332,86},13,Color::rgb(0x0963d4),3,cyan);txt(ui,font,atlas,menu[i],79,247+i*101,26,i==selected?white:muted);}
  box(ui,412,165,740,545);box(ui,1170,165,720,545);
  if(selected==0){
   ui.arc(785,431,260,13,3.55f,8.96f,blue);
   ui.arc(785,431,244,7,3.55f,8.96f,purple);
   ui.circle(785,431,173,Color::rgb(0x0c2c69));
   ui.circle(785,431,167,Color::rgb(0x04142f));
   txt(ui,font,atlas,s.running?"TESTING":s.completed?"FINISHED":"START",785,438,45,white,Align::center);
   txt(ui,font,atlas,s.running?"PLEASE WAIT":s.completed?"X TO REPEAT":"X SPEED TEST",785,487,21,muted,Align::center);
   for(int i=0;i<3;i++){float x=450+i*232;box(ui,x,608,216,82);txt(ui,font,atlas,i==0?"Download":i==1?"Upload":"Latency",x+18,640,20,i==0?cyan:i==1?purple:green);
    txt(ui,font,atlas,fmt(i==0?s.down:i==1?s.up:s.ping)+(i==2?" ms":" Mbps"),x+18,677,25);}
  }else{
   txt(ui,font,atlas,menu[selected],455,248,38);
   txt(ui,font,atlas,selected==1?"Current session results (not saved)":selected==2?"Testing HTTPS endpoint: speed.cloudflare.com":"Native PS5 internet benchmarking utility",455,306,23,muted);
   if(selected==1){txt(ui,font,atlas,s.completed?"Download: "+fmt(s.down)+" Mbps  |  Upload: "+fmt(s.up)+" Mbps":"No completed test yet",455,382,26);}
   if(selected==2)txt(ui,font,atlas,"ISP, public IP and NAT are not collected in v0.1.0",455,365,22,muted);
  }
  txt(ui,font,atlas,"LIVE SPEED GRAPH",1214,225,29);
  for(int i=0;i<5;i++)ui.line(1232,287+i*72,1840,287+i*72,1,Color::rgb(0x164174));
  graph(ui,s.downGraph,{1232,298,600,298},cyan,1000);
  graph(ui,s.upGraph,{1232,298,600,298},purple,1000);
  txt(ui,font,atlas,"DOWNLOAD",1230,657,18,cyan);txt(ui,font,atlas,"UPLOAD",1480,657,18,purple);
  box(ui,412,725,1478,250);
  txt(ui,font,atlas,"TEST SERVER",455,787,28);
  txt(ui,font,atlas,"Cloudflare global edge (automatic routing)",455,830,25,cyan);
  txt(ui,font,atlas,"HTTPS  |  Verified TLS  |  RTT includes connection setup",455,868,21,muted);
  txt(ui,font,atlas,"STATUS",1200,787,25);
  txt(ui,font,atlas,s.status,1200,834,20,muted);
  if(s.running)ui.rounded_rect({1195,875,610*(float)s.progress,11},5,cyan);
  txt(ui,font,atlas,"X Start test     D-Pad Navigate     O Back / Exit     Triangle Home",68,1038,23,muted);
  renderer.begin();renderer.draw(ui);renderer.present(0,display.width(),display.height());display.swap();hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
