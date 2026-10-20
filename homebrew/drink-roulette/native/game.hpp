// SPDX-License-Identifier: GPL-3.0-or-later
// Drink Roulette by YAHBALLON — portable, testable offline game logic.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace roulette {
constexpr float pi=3.14159265358979323846f;
constexpr float tau=2*pi;
enum class Screen { SetupCount, SetupNames, EditName, SetupMode, SetupPack, SetupSummary, Home, Players, Rules, Challenges, Settings, Game, Result, Exit };
enum class Category { Truth, Dare, Shot, Pass, Cheers, GroupDrink, Drink };
struct Slice { Category category; const char* label; uint32_t color; };
constexpr std::array<Slice,10> slices{{
 {Category::Truth,"TRUTH",0xffc62f},{Category::Dare,"DARE",0xc452ff},
 {Category::Shot,"SHOT",0x21a9ff},{Category::Pass,"PASS",0x19ed86},
 {Category::Cheers,"CHEERS",0xff8d27},{Category::GroupDrink,"GROUP DRINK",0xff335c},
 {Category::Dare,"DARE",0xca3fff},{Category::Truth,"TRUTH",0xffd840},
 {Category::Shot,"SHOT",0x2d96ff},{Category::Drink,"DRINK",0xff335f}
}};
struct Prompt {Category category;const char* text;int level=0;uint32_t mode_mask=31;};
} // namespace roulette
#include "challenges.hpp"
namespace roulette {
constexpr std::array<const char*,12> name_pool{{"Alex","Jamie","Taylor","Morgan","Charlie","Sam","Jordan","Casey","Riley","Avery","Sky","Robin"}};
struct Player {std::string name;bool ready=true;int turns=0;};
struct Game {
 Screen screen=Screen::SetupCount;
 bool setup_complete=false;
 int pack=2; // 0 Light, 1 Normal, 2 All (433 distinct challenges)
 int editing_player=0;
 int keyboard_cursor=0;
 int setup_focus=0;
 Screen keyboard_return=Screen::SetupNames;
 std::string editing_name;
 std::array<Player,8> players{};
 int player_count=4;
 int active=0;
 int focus=0;
 int settings_focus=0;
 int preview=0;
 int rounds=30;
 int turn=0;
 int mode=0; // Standard, Wild, Couples, Quickfire, Truth & Dare
 bool spinning=false;
 float spin_start_angle=0,angle=0,spin_end_angle=0,spin_elapsed=0,spin_duration=4.0f;
 int pending_slice=0;
 int winning_slice=0;
 int prompt_index=0;
 uint64_t random_state=0x9e3779b97f4a7c15ULL;
 std::string banner="All challenges are optional; non-alcoholic drinks welcome.";
 std::array<int,64> prompt_history{};
 int history_next=0;
 int history_size=0;
 Game(){for(int i=0;i<8;i++)players[i].name="Player "+std::to_string(i+1);}
 void seed(uint64_t s){random_state=s?s:0x9e3779b97f4a7c15ULL;}
 uint32_t random(){uint64_t x=random_state;x^=x>>12;x^=x<<25;x^=x>>27;random_state=x;return uint32_t((x*2685821657736338717ULL)>>32);}
 int active_count() const {return player_count;}
 bool all_ready() const {for(int i=0;i<player_count;i++)if(!players[i].ready)return false;return true;}
 int slice_at(float radians) const {
   float n=std::fmod(radians+pi/10,tau);if(n<0)n+=tau;
   return std::clamp(int(n/(tau/10)),0,9);
 }
 bool category_allowed(Category cat) const {
   if(mode==3)return cat==Category::Shot;
   if(mode==4)return cat==Category::Truth||cat==Category::Dare;
   if(mode==2)return cat==Category::Truth||cat==Category::Dare||cat==Category::Cheers||cat==Category::GroupDrink;
   return true;
 }
 bool prompt_available(const Prompt& p) const {return p.level<=pack&&category_allowed(p.category)&&(p.mode_mask&(1u<<mode));}
 int available_prompt_count() const {int n=0;for(const auto& p:prompts)if(prompt_available(p))n++;return n;}
 bool recent(int i) const {for(int j=0;j<history_size;j++)if(prompt_history[j]==i)return true;return false;}
 int random_prompt(Category category){
   std::array<int,prompts.size()> options{};int n=0;
   for(int i=0;i<int(prompts.size());++i)if(prompts[i].category==category&&prompt_available(prompts[i])&&!recent(i))options[n++]=i;
   if(n==0){history_size=0;for(int i=0;i<int(prompts.size());++i)if(prompts[i].category==category&&prompt_available(prompts[i]))options[n++]=i;}
   if(n==0)return 0;
   const int index=options[random()%uint32_t(n)];
   prompt_history[history_next]=index;history_next=(history_next+1)%int(prompt_history.size());
   history_size=std::min(int(prompt_history.size()),history_size+1);
   return index;
 }
 void begin_name_edit(int i,Screen ret){
   editing_player=std::clamp(i,0,player_count-1);keyboard_return=ret;
   editing_name=players[editing_player].name;
   if(editing_name.rfind("Player ",0)==0)editing_name.clear();
   keyboard_cursor=0;screen=Screen::EditName;
 }
 void apply_name_edit(){
   if(editing_name.empty())editing_name="Player "+std::to_string(editing_player+1);
   players[editing_player].name=editing_name.substr(0,16);
   screen=keyboard_return;
 }
 void start_new_setup(){setup_complete=false;screen=Screen::SetupCount;setup_focus=0;spinning=false;}
 bool slice_allowed(int i) const {return category_allowed(slices[i].category);}
 void start_session(){
   if(!all_ready()){screen=Screen::Players;banner="Set every active player to Ready before starting.";return;}
   screen=Screen::Game;turn=0;active=0;spinning=false;angle=0;history_size=0;history_next=0;
   for(auto& p:players)p.turns=0;
   banner="Press X to spin the bottle!";
 }
 void start_spin(){
   if(spinning||screen!=Screen::Game)return;
   std::array<int,slices.size()> options{};int n=0;
   for(int i=0;i<int(slices.size());i++)if(slice_allowed(i))options[n++]=i;
   pending_slice=options[random()%uint32_t(n)];
   spin_start_angle=angle;
   const float current=std::fmod(angle,tau);
   const float desired=float(pending_slice)*tau/float(slices.size());
   float delta=desired-current;if(delta<0)delta+=tau;
   spin_end_angle=angle+(5+int(random()%4))*tau+delta;
   spin_duration=3.5f+float(random()%130)/100.f;
   spin_elapsed=0;spinning=true;banner="Spinning...";
 }
 bool update(float dt){
   if(!spinning)return false;
   spin_elapsed=std::min(spin_duration,spin_elapsed+std::clamp(dt,0.f,0.05f));
   const float t=spin_elapsed/spin_duration;
   const float eased=1.f-(1.f-t)*(1.f-t)*(1.f-t)*(1.f-t);
   angle=spin_start_angle+(spin_end_angle-spin_start_angle)*eased;
   if(t<1.f)return false;
   spinning=false;
   angle=std::fmod(spin_end_angle,tau);
   winning_slice=slice_at(angle);
   prompt_index=random_prompt(slices[winning_slice].category);
   screen=Screen::Result;
   banner="Challenge revealed! Press X for the next turn.";
   return true;
 }
 void next_turn(){
   if(screen!=Screen::Result)return;
   ++players[active].turns;++turn;active=(active+1)%player_count;
   screen=Screen::Game;
   banner=(turn>=rounds&&rounds>0)?"Session complete! Triangle to restart; X to keep playing.":"Press X to spin the bottle!";
 }
 void cycle_name(int p){
   auto it=std::find_if(name_pool.begin(),name_pool.end(),[&](const char* n){return players[p].name==n;});
   int i=it==name_pool.end()?0:int(it-name_pool.begin());
   players[p].name=name_pool[(i+1)%name_pool.size()];
 }
};
} // namespace roulette
