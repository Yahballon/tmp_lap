// SPDX-License-Identifier: GPL-3.0-or-later
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
enum class Screen { Home, Players, Rules, Challenges, Settings, Game, Result, Exit };
enum class Category { Truth, Dare, Shot, Pass, Cheers, GroupDrink, Drink };
struct Slice { Category category; const char* label; uint32_t color; };
constexpr std::array<Slice,10> slices{{
 {Category::Truth,"TRUTH",0xffc62f},{Category::Dare,"DARE",0xc452ff},
 {Category::Shot,"SHOT",0x21a9ff},{Category::Pass,"PASS",0x19ed86},
 {Category::Cheers,"CHEERS",0xff8d27},{Category::GroupDrink,"GROUP DRINK",0xff335c},
 {Category::Dare,"DARE",0xca3fff},{Category::Truth,"TRUTH",0xffd840},
 {Category::Shot,"SHOT",0x2d96ff},{Category::Drink,"DRINK",0xff335f}
}};
struct Prompt {Category category;const char* text;};
constexpr std::array<Prompt,48> prompts{{
 {Category::Truth,"What was your most embarrassing first impression?"},
 {Category::Truth,"What is a secret talent nobody here knows?"},
 {Category::Truth,"Who in this room knows you best?"},
 {Category::Truth,"What is the funniest lie you've ever told?"},
 {Category::Truth,"What's your most ridiculous purchase?"},
 {Category::Truth,"What's the weirdest dream you've had?"},
 {Category::Truth,"What song do you secretly love?"},
 {Category::Truth,"What's your funniest childhood memory?"},
 {Category::Truth,"When was the last time you laughed until you cried?"},
 {Category::Truth,"What is something you want to learn?"},
 {Category::Dare,"Do your best celebrity impression for 15 seconds."},
 {Category::Dare,"Speak in rhyme until your next turn."},
 {Category::Dare,"Tell a joke without smiling."},
 {Category::Dare,"Sing the chorus of a favorite song."},
 {Category::Dare,"Act like a robot for 20 seconds."},
 {Category::Dare,"Make up a dance move; everyone copies you."},
 {Category::Dare,"Pretend you are a TV news reporter."},
 {Category::Dare,"Talk with an accent for one round."},
 {Category::Dare,"Invent a dramatic movie title about this party."},
 {Category::Dare,"Give someone a ridiculously fancy compliment."},
 {Category::Drink,"Take a small sip of your chosen drink, or pass."},
 {Category::Drink,"Make a toast; anyone who wants to may sip."},
 {Category::Drink,"Name a fruit in three seconds or take a water break."},
 {Category::Drink,"Choose someone to raise a glass with you."},
 {Category::Drink,"Say 'cheers' in another language."},
 {Category::Drink,"Tell your funniest party memory."},
 {Category::Shot,"Take a tiny sip, choose water, or skip this turn."},
 {Category::Shot,"Quick quiz: name three countries in five seconds."},
 {Category::Shot,"Do a mini toast to the person on your right."},
 {Category::Shot,"Name four animals that begin with different letters."},
 {Category::Shot,"Make a funny face for the group."},
 {Category::Shot,"Sip your drink or swap for a silly dare."},
 {Category::Pass,"Lucky! Skip your challenge without any penalty."},
 {Category::Pass,"You're safe. Give someone a compliment."},
 {Category::Pass,"Free pass! Choose the next player."},
 {Category::Pass,"Enjoy a break. Hydrate if you want."},
 {Category::Cheers,"Everyone raise a glass for the good times!"},
 {Category::Cheers,"Celebrate the funniest person in the room."},
 {Category::Cheers,"Share a group high-five or virtual cheers."},
 {Category::Cheers,"All together: best party chant wins."},
 {Category::Cheers,"Toast to your favorite memory together."},
 {Category::Cheers,"Make an imaginary award for one player."},
 {Category::GroupDrink,"Group toast! A sip is always optional."},
 {Category::GroupDrink,"Everyone share one fun fact about themselves."},
 {Category::GroupDrink,"Go around and name a favorite movie."},
 {Category::GroupDrink,"Everyone vote on the best dance move."},
 {Category::GroupDrink,"Do a synchronized cheer as a group."},
 {Category::GroupDrink,"Each player tells a one-word story, in order."}
}};
constexpr std::array<const char*,12> name_pool{{"Alex","Jamie","Taylor","Morgan","Charlie","Sam","Jordan","Casey","Riley","Avery","Sky","Robin"}};
struct Player {std::string name;bool ready=true;int turns=0;};
struct Game {
 Screen screen=Screen::Home;
 std::array<Player,8> players{};
 int player_count=4;
 int active=0;
 int focus=0;
 int settings_focus=0;
 int preview=0;
 int rounds=15;
 int turn=0;
 int mode=0;
 bool spinning=false;
 float spin_start_angle=0,angle=0,spin_end_angle=0,spin_elapsed=0,spin_duration=4.0f;
 int pending_slice=0;
 int winning_slice=0;
 int prompt_index=0;
 uint64_t random_state=0x9e3779b97f4a7c15ULL;
 std::string banner="Everyone can pass; non-alcoholic drinks welcome.";
 Game(){for(int i=0;i<8;i++)players[i].name=name_pool[i];}
 void seed(uint64_t s){random_state=s?s:0x9e3779b97f4a7c15ULL;}
 uint32_t random(){uint64_t x=random_state;x^=x>>12;x^=x<<25;x^=x>>27;random_state=x;return uint32_t((x*2685821657736338717ULL)>>32);}
 int active_count() const {return player_count;}
 bool all_ready() const {for(int i=0;i<player_count;i++)if(!players[i].ready)return false;return true;}
 int slice_at(float radians) const {
   float n=std::fmod(radians+pi/10,tau);if(n<0)n+=tau;
   return std::clamp(int(n/(tau/10)),0,9);
 }
 int random_prompt(Category category){
   std::array<int,prompts.size()> options{};int n=0;
   for(int i=0;i<int(prompts.size());++i)if(prompts[i].category==category)options[n++]=i;
   return n?options[random()%uint32_t(n)]:0;
 }
 void start_session(){
   if(!all_ready()){screen=Screen::Players;banner="Set every active player to Ready before starting.";return;}
   screen=Screen::Game;turn=0;active=0;spinning=false;angle=0;
   for(auto& p:players)p.turns=0;
   banner="Press X to spin the bottle!";
 }
 void start_spin(){
   if(spinning||screen!=Screen::Game)return;
   pending_slice=int(random()%slices.size());
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
}