// SPDX-License-Identifier: GPL-3.0-or-later
// MESH CLOUD by YAHBALLON v0.1.0 - native PS5 local storage dashboard.
// Native console UI, not a live peer or distributed storage provider.
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "core/input.hpp"
#include "diagnostics.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <string>
#include <vector>
#include <sys/stat.h>
using namespace hui::gfx;
namespace {
const Color white=Color::rgb(0xecf5ff),blue=Color::rgb(0x399fff);
const Color muted=Color::rgb(0x96b0d2),green=Color::rgb(0x53f0b1);
const Color warning=Color::rgb(0xffbc64);
constexpr const char* version="v0.1.0";
std::string load_font(const char* path) {
 FILE* f=std::fopen(path,"rb");if(!f)return {};
 if(std::fseek(f,0,SEEK_END)!=0){std::fclose(f);return {};}
 const long length=std::ftell(f);
 if(length<1||length>16000000){std::fclose(f);return {};}
 std::rewind(f);
 std::string text(static_cast<std::size_t>(length),'\0');
 const auto n=std::fread(text.data(),1,text.size(),f);
 std::fclose(f);
 if(n!=text.size())return {};return text;
}
void append_log(const char* action) {
 const char* dir="/data/PS5MeshCloud";
 mkdir(dir,0700);
 FILE* file=std::fopen("/data/PS5MeshCloud/meshcloud-ps5.log","ab");
 if(!file)return;
 std::fprintf(file,"%s %s\n",version,action);
 std::fclose(file);
}
std::string cut(const std::string& str,std::size_t max) {
 return str.size()>max?str.substr(0,max>3?max-3:0)+"...":str;
}
void label(DrawList& d,const Font& f,unsigned atlas,const std::string& str,
 float x,float y,int size=20,Color color=white,Align align=Align::left) {
 d.text(f,atlas,str,x,y,static_cast<float>(size),color,align);
}
void panel(DrawList& d,float x,float y,float width,float height) {
 d.shadow({x,y,width,height},22,9,Color::rgb(0x147bff,.07f));
 d.bordered_rect({x,y,width,height},16,Color::rgb(0x0b1d37),1.2f,Color::rgb(0x20517a));
}
void bar(DrawList& d,const Font& f,unsigned atlas,const std::string& name,
 float x,float y,float w) {
 d.rounded_rect({x+1,y+1,w-2,55},13,Color::rgb(0x102c54));
 label(d,f,atlas,name,x+22,y+37,24);
}
void metric(DrawList& d,const Font& f,unsigned atlas,const std::string& heading,
 const std::string& value,float x,float y,float w,bool emphasis=false) {
 panel(d,x,y,w,160);
 label(d,f,atlas,heading,x+24,y+43,20,muted);
 label(d,f,atlas,cut(value,44),x+24,y+102,30,emphasis?green:white);
}
void nav(DrawList& d,const Font& f,unsigned atlas,int page) {
 const char* items[]={"Overview","Storage","Local files","Network","About"};
 panel(d,20,110,292,906);
 label(d,f,atlas,"WORKSPACE",45,153,19,muted);
 for(int i=0;i<5;i++) {
  const float y=194+i*109.f;
  if(page==i) {
   d.shadow({35,y,260,75},20,12,Color::rgb(0x076bf6,.26f));
   d.bordered_rect({35,y,260,75},12,Color::rgb(0x174eaa),2,blue);
  }else {
   d.rounded_rect({35,y,260,75},12,Color::rgb(0x102743));
  }
  d.circle(61,y+38,10,page==i?green:Color::rgb(0x5c81b1));
  label(d,f,atlas,items[i],84,y+46,23,page==i?white:muted);
 }
 label(d,f,atlas,"10 GiB MINIMUM",48,925,19,white);
 label(d,f,atlas,"NO GUEST ACCESS",48,961,16,warning);
}
void value_row(DrawList& d,const Font& f,unsigned atlas,const std::string& name,
 const std::string& value,float x,float y,float right,Color status=white) {
 label(d,f,atlas,name,x,y,20,muted);
 label(d,f,atlas,cut(value,58),right,y,21,status,Align::right);
 d.line(x,y+13,right,y+13,1,Color::rgb(0x284365));
}
void display_overview(DrawList& d,const Font& f,unsigned atlas,
 const meshps5::Snapshot& snapshot,int active) {
 const auto& chosen=snapshot.disks[static_cast<std::size_t>(active)];
 metric(d,f,atlas,"NETWORK MEMBERSHIP","LOCKED",338,131,480);
 metric(d,f,atlas,"LOCAL SPACE REQUIREMENT","10 GiB + 2 GiB margin",836,131,489);
 metric(d,f,atlas,"LOCAL FILES",std::to_string(snapshot.files.size()),1344,131,550);
 panel(d,338,311,993,466);bar(d,f,atlas,"LOCAL STORAGE STATUS",338,311,993);
 value_row(d,f,atlas,"Selected mount",chosen.name,371,424,1297);
 value_row(d,f,atlas,"Mount path",chosen.path,371,477,1297);
 value_row(d,f,atlas,"Filesystem readable",chosen.mounted?"YES":"NO",371,530,1297,
           chosen.mounted?green:warning);
 value_row(d,f,atlas,"Reported available bytes",
           chosen.space_reported?meshps5::human_bytes(chosen.available_bytes):"Unavailable",
           371,583,1297);
 value_row(d,f,atlas,"10 GiB local preflight",
           chosen.possible_local_preflight?"PASS (disk only)":"NOT PASSED",371,636,1297,
           chosen.possible_local_preflight?green:warning);
 label(d,f,atlas,"A free-space reading is NOT remote proof of contribution.",372,722,18,warning);
 panel(d,1348,311,546,466);bar(d,f,atlas,"PROTECTION",1348,311,546);
 label(d,f,atlas,"ENCRYPTED STORAGE",1376,425,22,white);
 label(d,f,atlas,"Desktop local vault: AES-256-GCM",1377,461,19,muted);
 label(d,f,atlas,"CIDv1 integrity, 90-day expiry",1377,507,19,muted);
 label(d,f,atlas,"PS5 crypto port: NOT BUILT",1377,550,19,warning);
 label(d,f,atlas,"Audit + P2P: NOT ENABLED",1377,594,19,warning);
 label(d,f,atlas,"No silent network connections",1377,690,19,green);
 panel(d,338,795,1556,212);bar(d,f,atlas,"RELEASE STATUS",338,795,1556);
 label(d,f,atlas,"PS5 native interface / read-only diagnostics",372,898,25);
 label(d,f,atlas,"File transfers remain disabled until verified 10-GiB audits and transport exist.",
       372,951,18,warning);
}
void display_storage(DrawList& d,const Font& f,unsigned atlas,
 const meshps5::Snapshot& s,int active) {
 panel(d,338,131,1556,872);bar(d,f,atlas,"STORAGE MOUNTS - SQUARE TO SWITCH",338,131,1556);
 for(std::size_t i=0;i<s.disks.size();i++) {
  const auto& disk=s.disks[i];
  const float y=244+static_cast<float>(i)*196;
  if(static_cast<int>(i)==active)
   d.bordered_rect({363,y,1490,169},13,Color::rgb(0x133965),2,blue);
  else d.rounded_rect({363,y,1490,169},13,Color::rgb(0x102841));
  label(d,f,atlas,disk.name,392,y+49,26);
  label(d,f,atlas,disk.path,395,y+93,19,muted);
  label(d,f,atlas,disk.mounted?"MOUNT FOUND":"NOT AVAILABLE",1760,y+52,21,
        disk.mounted?green:warning,Align::right);
  label(d,f,atlas,
        disk.space_reported?meshps5::human_bytes(disk.available_bytes)+" reported free":"Capacity unavailable",
        1760,y+95,22,white,Align::right);
  label(d,f,atlas,disk.possible_local_preflight?"Local 10+2 GiB check: PASS":"Local 10+2 GiB check: NO",
        1760,y+137,17,disk.possible_local_preflight?green:warning,Align::right);
 }
 label(d,f,atlas,"These are filesystem reports, never proof of usable storage allocated to MeshCloud.",
       374,921,19,warning);
}
void display_files(DrawList& d,const Font& f,unsigned atlas,
 const meshps5::Snapshot& s,int selected) {
 panel(d,338,131,1556,872);bar(d,f,atlas,"LOCAL FILES - READ ONLY",338,131,1556);
 label(d,f,atlas,cut(s.file_folder,125),374,234,19,muted);
 label(d,f,atlas,"No files are published, encrypted or shared by this PS5 build.",374,273,19,warning);
 if(s.files.empty()) {
  label(d,f,atlas,"No regular files detected in this folder",374,398,28);
  label(d,f,atlas,"Copy local files to the displayed path to see them listed.",374,452,19,muted);
  return;
 }
 const int begin=std::max(0,selected-9);
 const int end=std::min(static_cast<int>(s.files.size()),begin+11);
 for(int i=begin;i<end;i++) {
  const float y=324+static_cast<float>(i-begin)*57;
  if(i==selected)d.rounded_rect({365,y-27,1487,52},9,Color::rgb(0x1955a0));
  label(d,f,atlas,cut(s.files[static_cast<std::size_t>(i)].name,95),389,y+6,22);
  label(d,f,atlas,meshps5::human_bytes(s.files[static_cast<std::size_t>(i)].bytes),
        1835,y+6,20,muted,Align::right);
 }
 label(d,f,atlas,std::to_string(s.files.size())+" files listed (up to 350 per scan)",
       375,980,18,muted);
}
void display_network(DrawList& d,const Font& f,unsigned atlas) {
 panel(d,338,131,1556,872);bar(d,f,atlas,"NETWORK SECURITY / FAIL-CLOSED",338,131,1556);
 const char* headings[]={
  "Independent proof of >=10 GiB allocated storage",
  "Three trusted auditor signatures",
  "Fresh authenticated peer channel / TLS exporter",
  "Relay-only transport and CID lookup",
  "Encrypted uploads, downloads and DHT search",
  "Signed full-file manifests and 90-day leases"
 };
 const char* values[]={
  "NOT IMPLEMENTED ON PS5",
  "Verification exists in desktop core; no auditor network",
  "NOT IMPLEMENTED ON PS5",
  "NOT CONNECTED",
  "DISABLED FOR ALL USERS",
  "Publisher validation implemented in desktop core"
 };
 for(int i=0;i<6;i++) {
  const float y=269+static_cast<float>(i)*112;
  label(d,f,atlas,headings[i],378,y,22,white);
  label(d,f,atlas,values[i],378,y+40,20,warning);
  d.line(379,y+56,1830,y+56,1,Color::rgb(0x1d385b));
 }
 label(d,f,atlas,"Knowing a CID never bypasses network membership checks.",377,974,20,green);
}
void display_about(DrawList& d,const Font& f,unsigned atlas,
 const meshps5::Snapshot& snapshot) {
 panel(d,338,131,1556,872);bar(d,f,atlas,"ABOUT MESH CLOUD",338,131,1556);
 const char* lines[]={
  "MESH CLOUD PS5 - Native prototype v0.1.0",
  "Developer: YAHBALLON",
  "GitHub: Yahballon/tmp_lap/homebrew/ps5-meshcloud",
  "Title ID: PPSA99718",
  "Minimum contribution rule: 10 GiB independently audited",
  "File inactivity policy: 90 days from verified full download",
  "Current PS5 build: filesystem diagnostics, no transfer capability",
  "Desktop build: local encrypted vault and testable security components",
  "No Sony / PlayStation artwork or branding is used."
 };
 for(int i=0;i<9;i++)
  label(d,f,atlas,lines[i],375,263+static_cast<float>(i)*73,22,i>5?muted:white);
 label(d,f,atlas,cut(snapshot.error,110),374,961,19,warning);
}
}
int main() {
 hui::ps5::Display display;
 if(!display.open())hui::sys::park();
 hui::gfx::Renderer renderer;
 if(!renderer.init())hui::sys::park();
 Font font;
 auto data=load_font("/app0/assets/fonts/inter-semibold.huifont");
 if(data.empty())data=load_font("/data/homebrew/PPSA99718/assets/fonts/inter-semibold.huifont");
 if(!font.load(data))hui::sys::park();
 const unsigned atlas=renderer.batch().create_font_texture(font);
 hui::ps5::Pad pad;pad.open();
 append_log("BOOT native UI / all network access disabled");
 int page=0,active_disk=0,selected_file=0;
 auto snapshot=meshps5::collect(active_disk);
 unsigned last_buttons=0;unsigned long long nav_next=0;
 std::string notice="Read-only native PS5 prototype. Press TRIANGLE to refresh.";
 for(;;) {
  std::array<hui::PadSample,64> samples{};
  const auto count=pad.read(samples);
  unsigned buttons=0,pressed=0;bool connected=false;
  for(std::size_t i=0;i<count;i++) {
   if(!samples[i].connected)continue;
   connected=true;buttons=samples[i].buttons;
   if(samples[i].left_y<68)buttons|=hui::pad_bits::kUp;
   else if(samples[i].left_y>188)buttons|=hui::pad_bits::kDown;
   if(samples[i].left_x<68)buttons|=hui::pad_bits::kLeft;
   else if(samples[i].left_x>188)buttons|=hui::pad_bits::kRight;
   pressed|=buttons&~last_buttons;
   last_buttons=buttons;
  }
  if(!connected){buttons=0;last_buttons=0;}
  const auto tick=static_cast<unsigned long long>(hui::sys::monotonic_us());
  const auto arrows=buttons&(hui::pad_bits::kUp|hui::pad_bits::kDown|
                             hui::pad_bits::kLeft|hui::pad_bits::kRight);
  unsigned nav_button=pressed&arrows;
  if(nav_button)nav_next=tick+300000;
  else if(arrows && tick>=nav_next){nav_button=arrows;nav_next=tick+140000;}
  else if(!arrows)nav_next=0;
  auto hit=[&](unsigned bit){return (pressed&bit)!=0;};
  if(nav_button&hui::pad_bits::kLeft)page=(page+4)%5;
  if(nav_button&hui::pad_bits::kRight)page=(page+1)%5;
  if(hit(hui::pad_bits::kL1))page=(page+4)%5;
  if(hit(hui::pad_bits::kR1))page=(page+1)%5;
  if(nav_button&hui::pad_bits::kUp) {
   if(page==2)selected_file=std::max(0,selected_file-1);
   else page=(page+4)%5;
  }
  if(nav_button&hui::pad_bits::kDown) {
   if(page==2)selected_file=std::min(std::max(0,static_cast<int>(snapshot.files.size())-1),
                                    selected_file+1);
   else page=(page+1)%5;
  }
  if(hit(hui::pad_bits::kTriangle)){
   snapshot=meshps5::collect(active_disk);
   selected_file=0;
   notice="Storage refreshed; peer networking remains locked.";
   append_log("Manual read-only refresh");
  }
  if(hit(hui::pad_bits::kSquare)){
   active_disk=(active_disk+1)%3;
   snapshot=meshps5::collect(active_disk);
   selected_file=0;
   notice="Selected a different local storage mount.";
  }
  if(hit(hui::pad_bits::kOptions))page=4;
  if(hit(hui::pad_bits::kCross)) {
   if(page==0)page=1;
   else if(page==1||page==2)notice="Read-only: no file sharing or crypto port is enabled.";
   else notice="All network actions are intentionally denied.";
  }
  if(hit(hui::pad_bits::kCircle)) {
   if(page!=0)page=0;
   else {pad.close();renderer.release();display.close();hui::sys::quit();}
  }
  DrawList d;
  d.gradient_rect({0,0,1920,1080},0,
                  Color::rgb(0x041027),Color::rgb(0x0b2144));
  d.arc(1830,180,720,2.5f,3.3f,6.1f,Color::rgb(0x087dff,.10f));
  d.rounded_rect({0,0,1920,98},0,Color::rgb(0x06162e));
  d.circle(64,48,24,Color::rgb(0x157cff));
  for(int i=0;i<4;i++)d.line(53+i*9,35,53+i*9,62,2,white);
  label(d,font,atlas,"MESH CLOUD",108,58,38,white);
  label(d,font,atlas,"BY YAHBALLON   /   PS5 NATIVE PROTOTYPE",400,53,22,muted);
  label(d,font,atlas,version,1890,56,20,white,Align::right);
  nav(d,font,atlas,page);
  if(page==0)display_overview(d,font,atlas,snapshot,active_disk);
  else if(page==1)display_storage(d,font,atlas,snapshot,active_disk);
  else if(page==2)display_files(d,font,atlas,snapshot,selected_file);
  else if(page==3)display_network(d,font,atlas);
  else display_about(d,font,atlas,snapshot);
  d.rounded_rect({0,1025,1920,55},0,Color::rgb(0x07172c));
  label(d,font,atlas,cut(notice,110),24,1058,17,warning);
  label(d,font,atlas,"L1/R1 TAB   D-PAD NAVIGATE   TRIANGLE RESCAN   SQUARE MOUNT   X SELECT   O BACK",
        1888,1058,16,white,Align::right);
  renderer.begin();renderer.draw(d);renderer.present(0,display.width(),display.height());
  display.swap();hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
