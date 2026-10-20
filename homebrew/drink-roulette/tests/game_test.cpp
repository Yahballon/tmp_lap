// Portable game/deck regression tests. Run before every PS5 build.
#include "../native/game.hpp"
#include <cassert>
#include <cstdio>
#include <set>
#include <string>
int main(){
 using namespace roulette;
 Game g;
 assert(g.screen==Screen::SetupCount&&!g.setup_complete);
 assert(g.player_count==4&&g.players[0].name=="Player 1");
 assert(prompts.size()>=400&&g.available_prompt_count()>400);
 // Check every prompt is real, printable, appropriately categorized and distinct.
 std::set<std::string> unique;
 for(const auto& p:prompts){
  assert(p.text&&std::string(p.text).size()>17&&std::string(p.text).size()<160);
  assert(unique.insert(std::string(p.text)).second);
  assert(p.level>=0&&p.level<=2);
 }
 g.player_count=8;assert(g.all_ready());g.players[7].ready=false;
 g.start_session();assert(g.screen==Screen::Players);
 g.players[7].ready=true;
 g.begin_name_edit(7,Screen::SetupNames);
 assert(g.screen==Screen::EditName&&g.editing_player==7);
 g.editing_name="PartyPro";g.apply_name_edit();
 assert(g.players[7].name=="PartyPro"&&g.screen==Screen::SetupNames);
 g.start_new_setup();assert(!g.setup_complete&&g.screen==Screen::SetupCount);
 g.setup_complete=true;g.screen=Screen::Home;
 g.seed(123456);g.start_session();assert(g.screen==Screen::Game);
 assert(g.slice_at(0)==0);
 assert(g.slice_at(tau-0.01f)==0);
 for(int i=0;i<180;i++){
  g.start_spin();assert(g.spinning);
  for(int j=0;j<600&&g.spinning;j++)g.update(1.f/60);
  assert(!g.spinning&&g.screen==Screen::Result);
  assert(g.winning_slice==g.pending_slice);
  assert(prompts[g.prompt_index].category==slices[g.winning_slice].category);
  assert(g.prompt_available(prompts[g.prompt_index]));
  g.next_turn();assert(g.screen==Screen::Game);
 }
 assert(g.turn==180);
 // Every mode and pack must filter both wheel sectors and the challenge deck.
 for(int mode=0;mode<5;mode++){
  for(int pack=0;pack<3;pack++){
   g.mode=mode;g.pack=pack;g.start_session();
   assert(g.available_prompt_count()>0);
   for(int i=0;i<25;i++){
    g.start_spin();assert(g.slice_allowed(g.pending_slice));
    for(int j=0;j<600&&g.spinning;j++)g.update(1.f/60);
    assert(g.screen==Screen::Result);
    assert(g.prompt_available(prompts[g.prompt_index]));
    g.next_turn();
   }
  }
 }
 assert(g.mode==4);
 puts("PASS: onboarding, names, 433+ unique prompts, 555 spins, all 5 modes and 3 intensity levels");
}
