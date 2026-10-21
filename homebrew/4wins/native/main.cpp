// SPDX-License-Identifier: GPL-3.0-or-later
// 4 WINS by YAHBALLON v0.1.2 — photo-based UI, ps5-homebrew-ui kit
#include "controls.hpp"
#include "board_layout.hpp"
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "core/input.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "vendor/stb_image.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <sys/stat.h>

using namespace hui::gfx;
using namespace wins4;
namespace {
constexpr int CHIP_SIZE=96;
constexpr const char* LABEL="4 WINS by YAHBALLON v0.1.2";
Color C(unsigned x,float a=1.f){return Color::rgb(x,a);}
std::string file(const std::string& path,size_t limit=18*1024*1024){
 FILE* f=fopen(path.c_str(),"rb");if(!f)return {};
 if(fseek(f,0,SEEK_END)){fclose(f);return {};}
 long n=ftell(f);if(n<=0||size_t(n)>limit){fclose(f);return {};}
 rewind(f);std::string bytes(size_t(n),'\0');if(fread(bytes.data(),1,bytes.size(),f)!=bytes.size())bytes.clear();
 fclose(f);return bytes;
}
std::string asset(const char* name,size_t limit=18*1024*1024){
 for(const char* prefix:{"/app0/assets/","/data/homebrew/PPSA99858/assets/","/mnt/usb0/homebrew/PPSA99858/assets/","assets/"}){
  auto data=file(std::string(prefix)+name,limit);if(!data.empty())return data;
 }
 return {};
}
void log(const char* s){
 mkdir("/data/4WINS",0755);mkdir("/data/4WINS/logs",0755);
 if(FILE* f=fopen("/data/4WINS/logs/4wins.log","ab")){fprintf(f,"[4 WINS v0.1.2] %s\n",s);fclose(f);}
}
void save(const Game& g){
 mkdir("/data/4WINS",0755);
 FILE* f=fopen("/data/4WINS/settings.tmp","wb");if(!f)return;
 fprintf(f,"mode=%d\nlevel=%d\nfirst=%d\nrumble=%d\nscore1=%d\nscore2=%d\ndraw=%d\n",int(g.opponent),int(g.level),int(g.first_player_red),int(g.sound),g.wins[0],g.wins[1],g.draws);
 if(fclose(f)==0)rename("/data/4WINS/settings.tmp","/data/4WINS/settings.ini");
}
void load(Game& g){
 FILE* f=fopen("/data/4WINS/settings.ini","rb");if(!f)return;
 char line[96];int n=0;
 while(fgets(line,sizeof(line),f)){
  if(sscanf(line,"mode=%d",&n)==1)g.opponent=Opponent(std::clamp(n,0,1));
  else if(sscanf(line,"level=%d",&n)==1)g.level=Level(std::clamp(n,0,2));
  else if(sscanf(line,"first=%d",&n)==1)g.first_player_red=n!=0;
  else if(sscanf(line,"rumble=%d",&n)==1)g.sound=n!=0;
  else if(sscanf(line,"score1=%d",&n)==1)g.wins[0]=std::clamp(n,0,100000);
  else if(sscanf(line,"score2=%d",&n)==1)g.wins[1]=std::clamp(n,0,100000);
  else if(sscanf(line,"draw=%d",&n)==1)g.draws=std::clamp(n,0,100000);
 }
 fclose(f);
}
struct Texture{unsigned id=0;};
Texture jpg(Renderer& renderer,const char* name){
 auto data=asset(name);if(data.empty()){log("Background file missing");return {};}
 int w=0,h=0,n=0;auto* px=stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(data.data()),int(data.size()),&w,&h,&n,4);
 if(!px||w<1||h<1||w>4096||h>4096){if(px)stbi_image_free(px);log("Background decode failed");return {};}
 auto id=renderer.batch().create_texture(w,h,px);stbi_image_free(px);return {id};
}
Texture chip(Renderer& renderer,const char* name){
 auto data=asset(name,CHIP_SIZE*CHIP_SIZE*4+1);
 if(data.size()!=CHIP_SIZE*CHIP_SIZE*4){log("Chip atlas missing");return {};}
 return {renderer.batch().create_texture(CHIP_SIZE,CHIP_SIZE,reinterpret_cast<const uint8_t*>(data.data()))};
}
void txt(DrawList& d,const Font& font,unsigned atlas,const std::string& s,float x,float y,float size,unsigned tint=0xffffff,Align align=Align::left){
 d.text(font,atlas,s,x,y,size,C(tint),align);
}
void rect(DrawList& d,float x,float y,float w,float h,float radius,unsigned fill,unsigned outline,float border=1.5f){
 d.bordered_rect({x,y,w,h},radius,C(fill),border,C(outline));
}
void mini_button(DrawList& d,const Font& f,unsigned atlas,int item,int focus,const std::string& title,float x,float y,float width,float height,unsigned accent){
 bool active=item==focus;
 if(active)d.glow({x,y,width,height},17,20,C(accent,.58f));
 d.bordered_rect({x,y,width,height},15,C(active?0x063c9b:0x060c23,.99f),active?3.f:1.6f,C(active?accent:0x27456f));
 if(active)d.rounded_rect({x+2,y+8,5,height-16},3,C(0x73ffff));
 txt(d,f,atlas,title,x+40,y+height*.64f,26,active?0xffffff:0xdfeaf9);
 if(active)txt(d,f,atlas,">",x+width-26,y+height*.65f,27,0x79f9ff,Align::right);
}
void main_menu(DrawList& d,const Font& f,unsigned atlas,const Game& g){
 const char* items[]={"Start Game","Game Mode","Settings","Exit"};
 for(int i=0;i<4;i++)mini_button(d,f,atlas,i,g.focus,items[i],69,306.f+96.f*i,395,90,0x31cfff);
 // Mode and controls belong to the menu; nothing is painted over board.
 rect(d,69,721,395,82,12,0x060e23,0x244c7e);
 txt(d,f,atlas,g.opponent==Opponent::Human?"2 PLAYERS - LOCAL":"1 PLAYER VS BOT",89,759,23,0xffdf78);
 txt(d,f,atlas,g.opponent==Opponent::Human?"Take turns using one controller":(g.level==Level::Easy?"BOT: EASY":g.level==Level::Medium?"BOT: MEDIUM":"BOT: HARD"),89,789,19,0xbbd2fa);
 txt(d,f,atlas,"D-PAD  Navigate     X  Select",74,1048,20,0xd4ecff);
}
// The reference photograph contains pre-rendered menu and player labels.
// One opaque architectural underlay per affected region is essential: a
// translucent material or slightly offset tile re-exposes the old texts.
// This runs exactly once before state-specific UI. The logo and board remain
// untouched. Bounds cover the original baked glow, not just its text.
void mask_embedded_ui(DrawList& d){
 d.rounded_rect({49,299,428,515},20,C(0x040b20));
 d.rounded_rect({484,875,460,154},22,C(0x050916));
 d.rounded_rect({982,875,462,154},22,C(0x050916));
}
void game_left(DrawList& d,const Font& f,unsigned atlas,const Game& g,const Controller& ctl){
 // The old menu is already hidden by mask_embedded_ui().
 rect(d,65,295,404,401,17,0x071634,g.turn==1?0xff2d64:0xffdd35,2.4f);
 const int actor=g.screen==Screen::Result?(g.winner==3?0:g.winner):g.turn;
 txt(d,f,atlas,g.screen==Screen::Result?(g.winner==3?"DRAW GAME":g.winner==1?"RED WINS!":"YELLOW WINS!"):g.screen==Screen::Paused?"GAME PAUSED":actor==1?"RED TO MOVE":"YELLOW TO MOVE",86,373,32,actor==2?0xffd74c:0xff4e67);
 d.line(86,391,442,391,2,C(0x274a81));
 if(g.opponent==Opponent::Bot&&g.turn==2&&g.screen==Screen::Playing)txt(d,f,atlas,"BOT IS THINKING...",86,443,23,0xffe65c);
 else txt(d,f,atlas,g.opponent==Opponent::Human?"2 PLAYERS - LOCAL":"1 PLAYER VS BOT",86,443,23,0xf6f9ff);
 txt(d,f,atlas,"LEFT / RIGHT   Select column",86,501,22,0xe7f3ff);
 txt(d,f,atlas,"X   Drop chip",86,546,23,0xe7f3ff);
 txt(d,f,atlas,"OPTIONS   Pause / Menu",86,591,21,0xacc7ea);
 d.line(86,610,442,610,1,C(0x395e97));
 txt(d,f,atlas,"SCORE",87,649,20,0x99caff);
 txt(d,f,atlas,std::to_string(g.wins[0])+"  :  "+std::to_string(g.wins[1]),442,652,29,0xffffff,Align::right);
 if(ctl.bot_pending&&g.turn==2)txt(d,f,atlas,"BOT",424,667,15,0xffd139,Align::right);
}
void draw_chip(DrawList& d,const Texture& tex,float cx,float cy,float sz,unsigned fallback){
 if(tex.id)d.image(tex.id,{cx-sz/2,cy-sz/2,sz,sz},kFullUv,C(0xffffff));
 else {d.circle(cx,cy,sz*.45f,C(fallback));d.ring(cx,cy,sz*.43f,5,C(0xffffff,.65f));}
}
void board(DrawList& d,const Font& f,unsigned atlas,const Game& g,const Controller& ctl,uint64_t now,const Texture& red,const Texture& yellow){
 const float player_size=kDiscSize;
 for(int c=0;c<COLS;c++)for(int r=0;r<ROWS;r++){
  int p=g.at(c,r);if(!p)continue;
  if(ctl.last_drop_col==c&&ctl.last_drop_row==r&&now-ctl.drop_time<400000ULL)continue;
  draw_chip(d,p==1?red:yellow,hole_x(c),hole_y(r),player_size,p==1?0xff1741:0xffcc21);
 }
 if(ctl.last_drop_col>=0&&now-ctl.drop_time<400000ULL){
  float elapsed=float(now-ctl.drop_time)/400000.f;
  float progress=1.f-std::pow(1.f-std::clamp(elapsed,0.f,1.f),3.f);
  const float target=hole_y(ctl.last_drop_row);
  const float y=195.f+(target-195.f)*progress;
  draw_chip(d,ctl.last_drop_owner==1?red:yellow,hole_x(ctl.last_drop_col),y,player_size,ctl.last_drop_owner==1?0xff1741:0xffcc21);
 }
 if(g.screen==Screen::Playing&&!(g.turn==2&&g.opponent==Opponent::Bot)){
  float x=hole_x(g.selected);
  d.glow({x-35,198,70,40},17,15,C(g.turn==1?0xff3862:0xffe036,.8f));
  d.triangle({x-20,204,40,28},C(g.turn==1?0xff426b:0xffdf3a),0,3.14159265f);
  txt(d,f,atlas,std::to_string(g.selected+1),x,196,21,g.turn==1?0xff859d:0xffec9d,Align::center);
 }
 if(g.screen==Screen::Result&&g.winner!=3){
  float pulse=.55f+.45f*std::sin(float(now%1500000ULL)*0.000004f);
  for(int j=0;j<4;j++){
   int col=g.winning_cells[j*2],row=g.winning_cells[j*2+1];
   if(col>=0&&row>=0)d.ring(hole_x(col),hole_y(row),38,4,C(0xffffff,pulse));
  }
 }
}
void footer(DrawList& d,const Font& f,unsigned atlas,const Game& g){
 const bool bot=g.opponent==Opponent::Bot;
 // Built-in labels are erased and the entire footer area is opaque.
 d.glow({492,865,438,147},18,13,C(0xff174d,.55f));
 rect(d,500,869,432,143,18,0x130716,0xff264e,3);
 d.circle(552,932,38,C(0xe50d3a));d.ring(552,932,36,5,C(0xff6785));
 txt(d,f,atlas,"PLAYER 1",607,915,29,0xff536a);
 txt(d,f,atlas,"RED  |  X DROP",609,952,21,0xffffff);
 d.glow({991,865,438,147},18,13,C(0xffdf25,.47f));
 rect(d,990,869,432,143,18,0x171306,0xfad82a,3);
 d.circle(1044,932,38,C(0xffd12b));d.ring(1044,932,36,5,C(0xfff2a9));
 txt(d,f,atlas,bot?"BOT":"PLAYER 2",1097,915,29,0xffdf4e);
 txt(d,f,atlas,bot?(g.level==Level::Easy?"EASY AI":g.level==Level::Medium?"MEDIUM AI":"HARD AI"):"YELLOW  |  X DROP",1098,952,21,0xffffff);
 txt(d,f,atlas,"4 WINS  v0.1.2  by YAHBALLON",1838,1047,19,0xd1e5ff,Align::right);
}
void overlay(DrawList& d,const Font& f,unsigned atlas,const Game& g,float clock){
 if(g.screen==Screen::Home||g.screen==Screen::Playing)return;
 // Selection pages and result dialogs appear *beside* the board. Unlike
 // v0.1.0, none of them ever overlap the hole grid.
 float x=1475,y=288,w=373,h=490;
 d.shadow({x,y,w,h},18,21,C(0x000006,.78f));
 rect(d,x,y,w,h,20,0x040f27,0x3094e9,2.6f);
 const char* title="";
 if(g.screen==Screen::Mode)title="GAME MODE";
 else if(g.screen==Screen::Difficulty)title="BOT DIFFICULTY";
 else if(g.screen==Screen::Settings)title="SETTINGS";
 else if(g.screen==Screen::Paused)title="PAUSED";
 else if(g.screen==Screen::Result)title="ROUND FINISHED";
 else title="EXIT GAME?";
 txt(d,f,atlas,title,x+27,y+63,27,0x8cdeff);
 d.line(x+26,y+84,x+w-27,y+84,2,C(0x2764b2));
 const bool mode=g.screen==Screen::Mode,diff=g.screen==Screen::Difficulty;
 const bool settings=g.screen==Screen::Settings,paused=g.screen==Screen::Paused;
 const bool result=g.screen==Screen::Result,exiting=g.screen==Screen::Exit;
 const int count=mode?2:diff?3:settings?4:paused?3:2;
 if(result){txt(d,f,atlas,g.winner==3?"IT'S A DRAW":g.winner==1?"RED WINS!":"YELLOW WINS!",x+26,y+124,29,g.winner==2?0xffd84e:0xff526d);}
 if(exiting)txt(d,f,atlas,"Return to PS5 homebrew?",x+26,y+133,21,0xe6f4ff);
 for(int n=0;n<count;n++){
  std::string value;
  if(mode)value=n==0?"2 PLAYERS":"PLAYER VS BOT";
  else if(diff)value=n==0?"EASY":n==1?"MEDIUM":"HARD";
  else if(settings){
   if(n==0)value=std::string("FIRST: ")+(g.first_player_red?"RED":"YELLOW");
   if(n==1)value=std::string("BOT: ")+(g.level==Level::Easy?"EASY":g.level==Level::Medium?"MEDIUM":"HARD");
   if(n==2)value=std::string("RUMBLE: ")+(g.sound?"ON":"OFF");
   if(n==3)value="RESET SCORES";
  }else if(paused)value=n==0?"RESUME":n==1?"RESTART":"MAIN MENU";
  else if(result)value=n==0?"REMATCH":"MAIN MENU";
  else value=n==0?"YES, EXIT":"CANCEL";
  const float top=y+(result||exiting?169.f:126.f)+n*76.f;
  bool active=n==g.submenu;
  if(active)d.glow({x+20,top,w-40,63},11,11,C(0x2d9cff,.55f));
  rect(d,x+21,top,w-42,63,11,active?0x094098:0x0b1b38,active?0x3ee4ff:0x335685,active?2.8f:1.4f);
  txt(d,f,atlas,value,x+41,top+43,active?25:24,0xf4f9ff);
 }
 txt(d,f,atlas,settings?"LEFT/RIGHT: CHANGE":"D-PAD: SELECT",x+26,y+h-49,18,0x9cbfeb);
 txt(d,f,atlas,"X CONFIRM  /  O BACK",x+26,y+h-21,16,0x9bb8e2);
 (void)clock;
}
void config_left(DrawList& d,const Font& f,unsigned atlas,const Game& g){
 const char* title=g.screen==Screen::Mode?"SELECT GAME MODE":g.screen==Screen::Difficulty?"SELECT AI LEVEL":g.screen==Screen::Settings?"PREFERENCES":"EXIT GAME";
 rect(d,64,302,406,264,16,0x08152f,0x318fe4,2.2f);
 txt(d,f,atlas,title,89,365,28,0xa5e8ff);
 d.line(90,386,441,386,2,C(0x2b4b7a));
 txt(d,f,atlas,"D-PAD  CHANGE SELECTION",90,438,21,0xd5e7ff);
 txt(d,f,atlas,"X  CONFIRM",90,478,21,0xd5e7ff);
 txt(d,f,atlas,"O  BACK",90,519,21,0xd5e7ff);
}
void render(DrawList& d,const Game& g,const Controller& ctl,const Font& f,unsigned atlas,const Texture& photo,const Texture& red,const Texture& yellow,uint64_t now){
 if(photo.id)d.image(photo.id,{0,0,1920,1080},kFullUv,C(0xffffff));
 else{d.gradient_rect({0,0,1920,1080},0,C(0x030b26),C(0x090d19));txt(d,f,atlas,"4 WINS",70,170,112,0x72baff);}
 mask_embedded_ui(d); // every frame and every screen, once; no ghost UI survives
 if(g.screen==Screen::Home){
  main_menu(d,f,atlas,g);
  footer(d,f,atlas,g);
 }else if(g.screen==Screen::Playing||g.screen==Screen::Paused||g.screen==Screen::Result){
  board(d,f,atlas,g,ctl,now,red,yellow);
  game_left(d,f,atlas,g,ctl);
  footer(d,f,atlas,g);
  if(g.screen!=Screen::Playing)overlay(d,f,atlas,g,float(now)/1000000.f);
 }else{
  // Configuration is modal: never draw the Home menu behind a dialog.
  config_left(d,f,atlas,g);
  footer(d,f,atlas,g);
  overlay(d,f,atlas,g,float(now)/1000000.f);
 }
}
}
int main(){
 log(LABEL);
 hui::ps5::Display display;if(!display.open())hui::sys::park();
 Renderer renderer;if(!renderer.init())hui::sys::park();
 auto fdata=asset("fonts/inter-semibold.huifont",5*1024*1024);
 Font font;if(!font.load(fdata)){log("Font unavailable");hui::sys::park();}
 unsigned atlas=renderer.batch().create_font_texture(font);
 Texture scene=jpg(renderer,"scene_game.jpg");
 Texture red=chip(renderer,"red.rgba"),yellow=chip(renderer,"yellow.rgba");
 hui::ps5::Pad pad;pad.open();
 hui::InputTracker tracker;
 hui::InputSettings prefs;prefs.repeat_delay_us=310000;prefs.repeat_interval_us=118000;tracker.set_settings(prefs);
 Game g;load(g);g.seed^=uint64_t(hui::sys::monotonic_us());
 Controller ctl;uint64_t previous=uint64_t(hui::sys::monotonic_us());
 for(;;){
  uint64_t now=uint64_t(hui::sys::monotonic_us());
  float dt=std::clamp(float(now-previous)/1000000.f,0.f,.06f);previous=now;
  std::array<hui::PadSample,64> samples{};const size_t count=pad.read(samples);
  const hui::InputFrame frame=tracker.update(std::span<const hui::PadSample>(samples.data(),count),now);
  const Input in{
   .up=frame.nav==hui::Direction::up,.down=frame.nav==hui::Direction::down,
   .left=frame.nav==hui::Direction::left,.right=frame.nav==hui::Direction::right,
   .confirm=frame.is_pressed(hui::Action::confirm),.back=frame.is_pressed(hui::Action::back),
   .triangle=frame.is_pressed(hui::Action::north),.options=frame.is_pressed(hui::Action::menu),
   .l1=frame.is_pressed(hui::Action::page_prev),.r1=frame.is_pressed(hui::Action::page_next)};
  const auto old_screen=g.screen;const int old_moves=g.moves;
  if(ctl.handle(g,in,now)){
   save(g);log("Exit requested");pad.close();
   if(scene.id)glDeleteTextures(1,&scene.id);
   if(red.id)glDeleteTextures(1,&red.id);
   if(yellow.id)glDeleteTextures(1,&yellow.id);
   renderer.release();display.close();hui::sys::quit();
  }
  if(g.moves!=old_moves){pad.set_light_bar(g.turn==1?240:255,g.turn==1?40:197,g.turn==1?69:46);if(g.sound)pad.rumble(.22f,.10f);}
  if(old_screen!=g.screen&&g.screen==Screen::Result){ctl.save_needed=true;if(g.sound)pad.rumble(.5f,.25f);}
  if(ctl.save_needed){save(g);ctl.save_needed=false;}
  DrawList ui;render(ui,g,ctl,font,atlas,scene,red,yellow,now);
  renderer.begin();renderer.draw(ui);renderer.present(0,display.width(),display.height());display.swap();
  hui::sys::hide_splash_screen();pad.tick(dt);hui::sys::sleep_us(16000);
 }
}
