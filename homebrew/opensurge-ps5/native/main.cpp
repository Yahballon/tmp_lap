// SPDX-License-Identifier: GPL-3.0-or-later
// Open Surge PS5 v0.2.1 - corrected masked terrain, character and pad diagnostic.
// NOT the full original Allegro/SurgeScript gameplay engine.
#include "gfx/renderer.hpp"
#include "level_preview.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "vendor/stb_image.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <sys/stat.h>

namespace {
using namespace hui::gfx;
constexpr const char* kVersion="0.2.1";
constexpr const char* kTitle="OPEN SURGE PS5 - COLLISION PREVIEW";
constexpr const char* kData="/data/OpenSurge";
constexpr const char* kLog="/data/OpenSurge/legacy-port.log";
Color C(unsigned hex,float opacity=1.f){return Color::rgb(hex,opacity);}
const Color WHITE=C(0xecf9ff),DIM=C(0xb0bfcf),ACCENT=C(0x66ebac),GOLD=C(0xfbd27e);

void log_message(const std::string& msg){
  mkdir(kData,0755);
  if(FILE* f=fopen(kLog,"ab")){
    fprintf(f,"[OpenSurge native level preview 0.2.1] %s\n",msg.c_str());
    fclose(f);
  }
}
std::string bytes(const std::string& filename,size_t limit){
  FILE* f=fopen(filename.c_str(),"rb");if(!f)return {};
  if(fseek(f,0,SEEK_END)){fclose(f);return {};}
  const long n=ftell(f);
  if(n<1 || static_cast<size_t>(n)>limit){fclose(f);return {};}
  rewind(f);
  std::string data(static_cast<size_t>(n),'\0');
  if(fread(data.data(),1,data.size(),f)!=data.size())data.clear();
  fclose(f);
  return data;
}
std::string asset(const char* filename,size_t limit=8*1024*1024){
  for(const char* p:{"/app0/assets/","/data/homebrew/PPSA99358/assets/",
                     "/mnt/usb0/homebrew/PPSA99358/assets/"}){
    auto value=bytes(std::string(p)+filename,limit);
    if(!value.empty())return value;
  }
  return {};
}
std::string resolve_root(){
  for(const char* root:{"/app0/assets/game/","/data/homebrew/PPSA99358/assets/game/",
                        "/mnt/usb0/homebrew/PPSA99358/assets/game/"}){
    if(FILE* f=fopen((std::string(root)+"surge.cfg").c_str(),"rb")){
      fclose(f);return root;
    }
  }
  return {};
}
struct Probe{std::string kind,path;bool ok=false;size_t first_read=0;};
std::vector<Probe> load_probes(){
  auto list=asset("probe_paths.txt",64*1024);
  std::vector<Probe> results;
  size_t p=0;
  while(p<list.size() && results.size()<12){
    const size_t e=list.find('\n',p),last=e==std::string::npos?list.size():e;
    std::string line=list.substr(p,last-p);
    const auto sep=line.find('\t');
    if(sep!=std::string::npos && sep>0 && sep<line.size()-1){
      std::string kind=line.substr(0,sep),rel=line.substr(sep+1);
      // Only accept relative paths published by our build-time manifest.
      if(rel.find("..")==std::string::npos && rel[0]!='/' && rel.size()<200)
        results.push_back({kind,rel});
    }
    p=last+1;
  }
  return results;
}
void run_probes(std::vector<Probe>& probes,const std::string& root){
  for(auto& row:probes){
    row.ok=false;row.first_read=0;
    if(root.empty())continue;
    const std::string full=root+row.path;
    if(FILE* f=fopen(full.c_str(),"rb")){
      char buffer[512];
      row.first_read=fread(buffer,1,sizeof(buffer),f);
      row.ok=row.first_read>0;
      fclose(f);
    }
    log_message(std::string(row.ok?"ASSET_READ_OK ":"ASSET_READ_FAIL ")+row.path);
  }
}
void txt(DrawList& ui,const Font& font,unsigned atlas,const std::string& text,
         float x,float y,float size,Color color=WHITE,Align align=Align::left){
  ui.text(font,atlas,text,x,y,size,color,align);
}
unsigned image(Renderer& renderer){
  std::string jpg=asset("surge-preview.jpg",12*1024*1024);
  if(jpg.empty()){log_message("PREVIEW_MISSING");return 0;}
  int w=0,h=0,n=0;
  auto* pixel=stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(jpg.data()),
                                   static_cast<int>(jpg.size()),&w,&h,&n,4);
  if(!pixel||w<8||h<8||w>4096||h>4096){
    if(pixel)stbi_image_free(pixel);log_message("PREVIEW_DECODE_FAIL");return 0;
  }
  const unsigned texture=renderer.batch().create_texture(w,h,pixel);
  stbi_image_free(pixel);
  return texture;
}
void draw(DrawList& ui,const Font& font,unsigned atlas,unsigned preview,
          const std::vector<Probe>& probes,const std::string& root,bool detail,bool exiting){
  ui.gradient_rect({0,0,1920,1080},0,C(0x071827),C(0x0b1b33));
  ui.rounded_rect({62,58,1794,164},22,C(0x112b36,.96f));
  txt(ui,font,atlas,"OPEN SURGE",94,142,74,WHITE);
  txt(ui,font,atlas,"by YAHBALLON  /  native PS5 / old DevBridge pipeline",101,192,25,ACCENT);
  txt(ui,font,atlas,"v0.2.1",1807,143,27,DIM,Align::right);
  ui.bordered_rect({64,249,884,687},18,C(0x102631,.96f),2,C(0x236a69));
  ui.bordered_rect({970,249,884,687},18,C(0x102631,.96f),2,C(0x236a69));
  txt(ui,font,atlas,"ORIGINAL GAME DATA",103,317,37,ACCENT);
  txt(ui,font,atlas,root.empty()?"GAME DATA NOT FOUND":"GAME ROOT FOUND",900,312,22,root.empty()?GOLD:ACCENT,Align::right);
  ui.line(105,340,911,340,2,C(0x336f6d));
  if(probes.empty()){
    txt(ui,font,atlas,"Missing probe_paths.txt",110,437,29,GOLD);
  }else{
    for(size_t i=0;i<probes.size() && i<8;++i){
      const float y=369.f+i*67.f;
      const Probe& probe=probes[i];
      ui.rounded_rect({100,y,812,56},9,C(probe.ok?0x154636:0x50311e,.92f));
      txt(ui,font,atlas,probe.ok?"OK":"ERR",126,y+38,23,probe.ok?ACCENT:GOLD);
      const auto label=probe.kind+": "+probe.path;
      txt(ui,font,atlas,label.substr(0,51),205,y+37,20,WHITE);
    }
  }
  if(preview){
    ui.image(preview,{999,275,824,430},{0,0,1,1},WHITE);
  }else{
    txt(ui,font,atlas,"SURGE ART PREVIEW MISSING",1012,474,32,GOLD);
  }
  txt(ui,font,atlas,detail?"LEGACY PS5 GRAPHICS STACK":"RUNTIME TEST",1009,760,28,ACCENT);
  txt(ui,font,atlas,detail?"Renderer: PS5-DevBridge / OpenGL":"Checks native graphics, controller and assets",1009,806,22,WHITE);
  txt(ui,font,atlas,detail?"Controller: DualSense / PS5 pad":"Original gameplay engine: NOT LINKED",1009,847,22,detail?WHITE:GOLD);
  txt(ui,font,atlas,detail?"Audio + level simulation: NOT LINKED":"Actual playable level: NOT IMPLEMENTED",1009,888,22,GOLD);
  txt(ui,font,atlas,"X  LEVEL PREVIEW   |   TRIANGLE  STATUS   |   O  EXIT",960,1005,24,WHITE,Align::center);
  if(exiting){
    ui.rounded_rect({0,0,1920,1080},0,C(0x020a12,.85f));
    ui.bordered_rect({565,340,790,360},18,C(0x102631),3,ACCENT);
    txt(ui,font,atlas,"EXIT OPEN SURGE TEST?",960,476,42,WHITE,Align::center);
    txt(ui,font,atlas,"X Confirm      O Cancel",960,569,28,ACCENT,Align::center);
  }
}
}

int main(){
  log_message("BOOT LEGACY PS5 NATIVE");
  hui::ps5::Display display;
  if(!display.open()){log_message("DISPLAY_OPEN_FAILED");hui::sys::park();}
  Renderer renderer;
  if(!renderer.init()){log_message("RENDERER_INIT_FAILED");hui::sys::park();}
  const auto font_bytes=asset("fonts/inter-semibold.huifont",6*1024*1024);
  Font font;
  if(!font.load(font_bytes)){log_message("FONT_LOAD_FAILED");hui::sys::park();}
  const unsigned atlas=renderer.batch().create_font_texture(font);
  const unsigned preview=image(renderer);
  hui::ps5::Pad pad;pad.open();
  std::string root=resolve_root();
  auto probes=load_probes();run_probes(probes,root);
  surgepreview::LevelPreview level;
  const bool level_ready=level.load(renderer,root);
  log_message(level_ready?
      "LEVEL_PREVIEW_READY original demo-1.lev brick sheet, magenta alpha and collision mask":
      std::string("LEVEL_PREVIEW_FAILED ")+level.error());
  if(level_ready) {
    char line[160];
    std::snprintf(line,sizeof(line),"LEVEL_SPAWN x=%.1f y=%.1f grounded=%d bricks=%zu",
                  level.player_x(),level.player_y(),level.grounded()?1:0,level.count());
    log_message(line);
  }
  bool detail=false,exiting=false,level_mode=false;
  uint64_t last_us=static_cast<uint64_t>(hui::sys::monotonic_us());
  uint32_t previous=0;
  for(;;){
    std::array<hui::PadSample,64> samples{};
    const size_t count=pad.read(samples);
    uint32_t pressed=0,held=0;bool connected=false;
    for(size_t i=0;i<count;++i){
      if(!samples[i].connected)continue;
      connected=true;
      uint32_t bits=samples[i].buttons;
      if(samples[i].left_x<73)bits|=hui::pad_bits::kLeft;
      if(samples[i].left_x>183)bits|=hui::pad_bits::kRight;
      held=bits;
    }
    if(!connected)held=0;
    // Only the most recent controller sample is authoritative. A key held
    // across multiple read() packets is one press, not repeated RESET events.
    pressed=held & ~previous;
    previous=held;
    const uint64_t now=static_cast<uint64_t>(hui::sys::monotonic_us());
    const float dt=std::clamp(float(now-last_us)/1000000.f,0.f,.040f);
    last_us=now;
    if(level_mode && !exiting){
      if(pressed&hui::pad_bits::kCircle){level_mode=false;log_message("LEVEL_PREVIEW_EXIT");}
      else{
        const int dir=int((held&hui::pad_bits::kRight)!=0)-int((held&hui::pad_bits::kLeft)!=0);
        const bool grounded_before=level.grounded();
        const bool jumping=(pressed&hui::pad_bits::kCross)!=0;
        level.update(dt,dir,jumping);
        if(jumping&&grounded_before)log_message("LEVEL_JUMP");
        if(!grounded_before&&level.grounded())log_message("LEVEL_LANDED");
        if(pressed&hui::pad_bits::kSquare){
          level.reset();
          char line[160];
          std::snprintf(line,sizeof(line),"LEVEL_RESET x=%.1f y=%.1f grounded=%d",
                        level.player_x(),level.player_y(),level.grounded()?1:0);
          log_message(line);
        }
      }
    }else if(exiting){
      if(pressed&hui::pad_bits::kCircle)exiting=false;
      if(pressed&hui::pad_bits::kCross){
        log_message("EXIT NATIVE");
        pad.close();
        renderer.release();display.close();
        hui::sys::quit();
      }
    }else{
      if(pressed&hui::pad_bits::kCross){
        if(level_ready){
          level_mode=true;level.reset();
          log_message("LEVEL_PREVIEW_START demo-1.lev, original masks and magenta alpha; still not full engine");
        }else{
          root=resolve_root();run_probes(probes,root);
          log_message(std::string("LEVEL_PREVIEW_UNAVAILABLE ")+level.error());
        }
      }
      if(pressed&hui::pad_bits::kTriangle)detail=!detail;
      if(pressed&hui::pad_bits::kOptions)detail=!detail;
      if(pressed&hui::pad_bits::kCircle)exiting=true;
    }
    DrawList ui;
    if(level_mode && level_ready){
      level.draw(ui);
      ui.rounded_rect({36,28,1090,122},14,C(0x072128,.90f));
      txt(ui,font,atlas,"OPEN SURGE  /  ORIGINAL DEMO-1 LEVEL",66,81,33,WHITE);
      txt(ui,font,atlas,"v0.2.1  -  corrected alpha + original mask collision; NOT complete game engine",66,125,21,GOLD);
      ui.rounded_rect({36,979,1830,73},12,C(0x072128,.94f));
      txt(ui,font,atlas,"STICK/D-PAD MOVE    X JUMP    SQUARE RESET    O BACK",73,1029,28,WHITE);
      char probe_line[180];
      std::snprintf(probe_line,sizeof(probe_line),
          "SURGE (%.0f, %.0f)  GROUND:%s  VY:%+.0f  VISIBLE TILES:%d",
          level.player_x(),level.player_y(),level.grounded()?"YES":"NO",
          level.velocity_y(),level.visible());
      ui.rounded_rect({1230,30,630,67},10,C(0x092431,.87f));
      txt(ui,font,atlas,probe_line,1240,72,19,ACCENT);
    }else{
      draw(ui,font,atlas,preview,probes,root,detail,exiting);
      if(!level_ready)
        txt(ui,font,atlas,std::string("LEVEL PREVIEW: ")+level.error(),103,905,19,GOLD);
    }
    renderer.begin();renderer.draw(ui);
    renderer.present(0,display.width(),display.height());
    display.swap();
    hui::sys::hide_splash_screen();
    hui::sys::sleep_us(16000);
  }
}
