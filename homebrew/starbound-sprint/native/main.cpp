// SPDX-License-Identifier: GPL-3.0-or-later
// Starbound Sprint by YAHBALLON v0.1.0
#include "game.hpp"
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
using namespace hui::gfx;
static std::string read_file(const char* p){FILE* f=fopen(p,"rb");if(!f)return {};fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<=0||n>16000000){fclose(f);return {};}std::string v((size_t)n,0);size_t k=fread(v.data(),1,v.size(),f);fclose(f);return k==v.size()?v:std::string{};}
static void t(DrawList& d,const Font& f,unsigned a,const std::string& s,float x,float y,float z,Color c=Color::rgb(0xffffff),Align align=Align::left){d.text(f,a,s,x,y,z,c,align);}
int main(){
 hui::ps5::Display screen;if(!screen.open())hui::sys::park();
 Renderer render;if(!render.init())hui::sys::park();
 Font font;std::string bytes=read_file("/app0/assets/fonts/inter-semibold.huifont");
 if(bytes.empty())bytes=read_file("/data/homebrew/PPSA99641/assets/fonts/inter-semibold.huifont");
 if(!font.load(bytes))hui::sys::park();
 unsigned atlas=render.batch().create_font_texture(font);
 hui::ps5::Pad pad;pad.open();sprint::Game g;unsigned previous=0;int frames=0;
 const unsigned palette[8]={0x092a4f,0x251340,0x16445d,0x52213b,0x0e344f,0x290c4d,0x501d36,0x122d4b};
 const unsigned ground[8]={0x38c5b3,0xb08bff,0xecc67b,0xf78d72,0x7be3f2,0xc679ff,0xf1a85a,0x83d8ee};
 for(;;){
   std::array<hui::PadSample,64> samples{};size_t n=pad.read(samples);
   unsigned pressed=0,held=previous;float stick=0;
   for(size_t i=0;i<n;i++){pressed|=samples[i].buttons&~held;held=samples[i].buttons;stick=((int)samples[i].left_x-128)/127.f;}
   previous=held;
   if(pressed&hui::pad_bits::kCircle){if(g.phase==sprint::Phase::menu){pad.close();render.release();screen.close();hui::sys::quit();}g.reset_to_menu();}
   if(pressed&hui::pad_bits::kOptions){if(g.phase==sprint::Phase::playing)g.set_pause(true);else if(g.phase==sprint::Phase::paused)g.set_pause(false);}
   if(pressed&hui::pad_bits::kCross){
     if(g.phase==sprint::Phase::menu||g.phase==sprint::Phase::game_over||g.phase==sprint::Phase::victory)g.new_run(0);
     else if(g.phase==sprint::Phase::paused)g.set_pause(false);
     else if(g.phase==sprint::Phase::stage_clear)g.next_stage();
   }
   if((pressed&hui::pad_bits::kTriangle)&&g.phase==sprint::Phase::menu)g.new_run(g.progress.unlocked);
   sprint::Control input;input.horizontal=std::fabs(stick)>.2f?stick:0;
   if(held&hui::pad_bits::kLeft)input.horizontal=-1;
   if(held&hui::pad_bits::kRight)input.horizontal=1;
   input.jump=(held&hui::pad_bits::kCross)!=0;input.sprint=(held&hui::pad_bits::kSquare)!=0;
   g.step(input);++frames;
   DrawList d;int idx=g.stage%8;
   d.gradient_rect({0,0,1920,1080},0,Color::rgb(palette[idx]),Color::rgb(0x061021));
   for(int i=0;i<70;i++){float x=std::fmod((float)(i*221)+20000.f-g.camera*.2f,1920.f);float y=165+(i*73)%720;d.circle(x,y,2+(i%3),Color::rgb(0xd7fcff,.45f));}
   float offset=100.f;
   d.push_clip({45,150,1830,792});
   int first=std::max(0,(int)(g.camera/sprint::kTile)-2);
   int last=std::min(sprint::kColumns-1,first+34);
   for(int r=0;r<sprint::kRows;r++)for(int c=first;c<=last;c++){
     float x=offset+c*sprint::kTile-g.camera,y=153+r*sprint::kTile;char tile=g.tiles[r][c];
     if(tile=='#'){d.rounded_rect({x,y,60,60},4,Color::rgb(0x153552));d.rounded_rect({x,y,60,12},3,Color::rgb(ground[idx]));}
     else if(tile=='='){d.rounded_rect({x,y,60,17},6,Color::rgb(ground[idx]));}
     else if(tile=='S'){d.rounded_rect({x+4,y+33,52,17},6,Color::rgb(0xf4a076));d.triangle({x+15,y+6,30,26},Color::rgb(0xffd777));}
   }
   for(const auto& p:g.pickups)if(!p.taken){
     float x=offset+p.pos.x-g.camera+15,y=153+p.pos.y+10;
     if(x<45||x>1875)continue;
     d.ring(x,y,13,4,Color::rgb(0xffd45a));d.star(x,y,7,Color::rgb(0xffffe1));
   }
   for(const auto& e:g.drones)if(e.alive){
     float x=offset+e.pos.x-g.camera,y=153+e.pos.y;if(x<0||x>1920)continue;
     d.rounded_rect({x,y,42,40},13,Color::rgb(0xf16d9c));d.circle(x+13,y+15,5,Color::rgb(0x15213e));d.circle(x+29,y+15,5,Color::rgb(0x15213e));
   }
   float finish=offset+g.finish_x()-g.camera;
   if(finish>40&&finish<1880){d.line(finish,575,finish,920,9,Color::rgb(0x64ffd1));d.star(finish,550,38,Color::rgb(0xffd45a));}
   if(g.invulnerable==0||(frames/6)%2==0){
     float x=offset+g.player.x-g.camera,y=153+g.player.y;
     d.rounded_rect({x+7,y+19,29,29},8,Color::rgb(0x16344f));
     d.rounded_rect({x+3,y+2,40,29},11,Color::rgb(0x63f5cd));
     d.rounded_rect({x+9,y+8,28,18},6,Color::rgb(0x112844));
     d.circle(x+(g.facing_right?30:17),y+17,4,Color::rgb(0xffe38b));
     d.circle(x+25,y,5,Color::rgb(0xffcf60));
     d.rounded_rect({x+8,y+43,13,10},3,Color::rgb(0xff9866));
     d.rounded_rect({x+26,y+43,13,10},3,Color::rgb(0xff9866));
   }
   d.pop_clip();
   d.rounded_rect({0,0,1920,95},0,Color::rgb(0x07182c));
   t(d,font,atlas,"STARBOUND SPRINT by YAHBALLON",55,57,29,Color::rgb(0xffd45a));
   t(d,font,atlas,"v0.1.0",1860,57,23,Color::rgb(0xffffff),Align::right);
   t(d,font,atlas,"WORLD "+std::to_string(g.stage+1)+": "+sprint::Game::stage_name(g.stage),55,132,24);
   t(d,font,atlas,"SCORE "+std::to_string(g.score),755,132,23);
   t(d,font,atlas,"CRYSTALS "+std::to_string(g.crystals),1125,132,23,Color::rgb(0xffd45a));
   t(d,font,atlas,"LIVES "+std::to_string(g.lives),1590,132,23);
   d.rounded_rect({45,962,1830,90},16,Color::rgb(0x08182d));
   t(d,font,atlas,"D-PAD / STICK Move     X Jump     SQUARE Sprint     OPTIONS Pause     O Back",960,1018,22,Color::rgb(0xa8c4de),Align::center);
   if(g.phase!=sprint::Phase::playing){
      d.rounded_rect({0,151,1920,795},0,Color::rgb(0x050d20,.75f));
      d.bordered_rect({440,258,1040,544},23,Color::rgb(0x122a47),3,Color::rgb(0x65edd2));
      const char* heading=g.phase==sprint::Phase::menu?"STARBOUND SPRINT":g.phase==sprint::Phase::paused?"PAUSED":g.phase==sprint::Phase::stage_clear?"WORLD COMPLETE":g.phase==sprint::Phase::game_over?"GAME OVER":"YOU WIN!";
      t(d,font,atlas,heading,960,390,59,Color::rgb(0xffd45a),Align::center);
      const char* helper=g.phase==sprint::Phase::menu?"An original eight-world cosmic platform adventure":g.phase==sprint::Phase::paused?"CROSS to Resume - CIRCLE to Menu":g.phase==sprint::Phase::stage_clear?"CROSS to Continue":"CROSS to Play Again";
      t(d,font,atlas,helper,960,484,28,Color::rgb(0xb2d8ec),Align::center);
      if(g.phase==sprint::Phase::menu){
        t(d,font,atlas,"CROSS - Start New Game",960,574,29);
        t(d,font,atlas,"TRIANGLE - Continue from World "+std::to_string(g.progress.unlocked+1),960,620,26,Color::rgb(0xffffff),Align::center);
        t(d,font,atlas,"BEST SCORE "+std::to_string(g.progress.best_score),960,700,24,Color::rgb(0xffd45a),Align::center);
      }else t(d,font,atlas,"CROSS - Confirm",960,615,30,Color::rgb(0xffffff),Align::center);
   }
   render.begin();render.draw(d);render.present(0,screen.width(),screen.height());screen.swap();hui::sys::hide_splash_screen();hui::sys::sleep_us(16000);
 }
}
