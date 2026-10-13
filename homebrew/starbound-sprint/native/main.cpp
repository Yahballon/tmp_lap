// SPDX-License-Identifier: GPL-3.0-or-later
// Starbound Sprint by YAHBALLON v0.4.0
#include "game.hpp"
#include "art_renderer.hpp"
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
 std::uint32_t spriteTexture=0;
 if(!sprint_art::load_atlas(render,spriteTexture))hui::sys::park();
 std::uint32_t backgroundTexture=0;
 if(!sprint_art::load_backgrounds(render,backgroundTexture))hui::sys::park();
 hui::ps5::Pad pad;pad.open();sprint::Game g;unsigned previous=0;int frames=0;bool suppressJumpUntilRelease=false;
 for(;;){
   std::array<hui::PadSample,64> samples{};size_t n=pad.read(samples);
   unsigned pressed=0,held=previous;float stick=0;
   for(size_t i=0;i<n;i++){pressed|=samples[i].buttons&~held;held=samples[i].buttons;stick=((int)samples[i].left_x-128)/127.f;}
   previous=held;
   if(pressed&hui::pad_bits::kCircle){if(g.phase==sprint::Phase::menu){pad.close();sprint_art::release_atlas(spriteTexture);sprint_art::release_atlas(backgroundTexture);render.release();screen.close();hui::sys::quit();}g.reset_to_menu();}
   if(pressed&hui::pad_bits::kOptions){if(g.phase==sprint::Phase::playing)g.set_pause(true);else if(g.phase==sprint::Phase::paused)g.set_pause(false);}
   if(pressed&hui::pad_bits::kCross){
     if(g.phase==sprint::Phase::menu||g.phase==sprint::Phase::game_over||g.phase==sprint::Phase::victory){g.new_run(0);suppressJumpUntilRelease=true;}
     else if(g.phase==sprint::Phase::paused){g.set_pause(false);suppressJumpUntilRelease=true;}
     else if(g.phase==sprint::Phase::stage_clear){g.next_stage();suppressJumpUntilRelease=true;}
   }
   if((pressed&hui::pad_bits::kTriangle)&&g.phase==sprint::Phase::menu)g.new_run(g.progress.unlocked);
   sprint::Control input;input.horizontal=std::fabs(stick)>.2f?stick:0;
   if(held&hui::pad_bits::kLeft)input.horizontal=-1;
   if(held&hui::pad_bits::kRight)input.horizontal=1;
   if(!(held&hui::pad_bits::kCross))suppressJumpUntilRelease=false;
   input.jump=(held&hui::pad_bits::kCross)!=0 && !suppressJumpUntilRelease;
   input.sprint=(held&hui::pad_bits::kSquare)!=0;
   g.step(input);++frames;
   DrawList d;
   sprint_art::render_world(d,g,spriteTexture,backgroundTexture,frames);
   d.rounded_rect({0,0,1920,95},0,Color::rgb(0x07182c));
   t(d,font,atlas,"STARBOUND SPRINT by YAHBALLON",55,57,29,Color::rgb(0xffd45a));
   t(d,font,atlas,"v0.4.0",1860,57,23,Color::rgb(0xffffff),Align::right);
   t(d,font,atlas,"WORLD "+std::to_string(g.stage+1)+": "+sprint::Game::stage_name(g.stage),55,132,24);
   t(d,font,atlas,"SCORE "+std::to_string(g.score),755,132,23);
   t(d,font,atlas,"CRYSTALS "+std::to_string(g.crystals),1010,132,23,Color::rgb(0xffd45a));
   t(d,font,atlas,"JUMPS "+std::to_string(g.jumps_remaining())+"/2",1370,132,23,Color::rgb(0x65f2df));
   t(d,font,atlas,"LIVES "+std::to_string(g.lives),1630,132,23);
   d.rounded_rect({45,962,1830,90},16,Color::rgb(0x08182d));
   t(d,font,atlas,"D-PAD / STICK MOVE       X  DOUBLE JUMP       SQUARE  SPRINT       OPTIONS  PAUSE       O  BACK",960,1018,22,Color::rgb(0xa8c4de),Align::center);
   if(g.phase!=sprint::Phase::playing){
      d.rounded_rect({0,151,1920,795},0,Color::rgb(0x050d20,.75f));
      d.bordered_rect({440,258,1040,544},23,Color::rgb(0x122a47),3,Color::rgb(0x65edd2));
      const char* heading=g.phase==sprint::Phase::menu?"STARBOUND SPRINT":g.phase==sprint::Phase::paused?"PAUSED":g.phase==sprint::Phase::stage_clear?"WORLD COMPLETE":g.phase==sprint::Phase::game_over?"GAME OVER":"YOU WIN!";
      t(d,font,atlas,heading,960,390,59,Color::rgb(0xffd45a),Align::center);
      const char* helper=g.phase==sprint::Phase::menu?"Explore eight pixel-art worlds with enemies, hazards and secrets":g.phase==sprint::Phase::paused?"CROSS to Resume - CIRCLE to Menu":g.phase==sprint::Phase::stage_clear?"CROSS to Continue":"CROSS to Play Again";
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
