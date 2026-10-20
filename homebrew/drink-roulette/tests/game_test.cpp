#include "../native/game.hpp"
#include <cassert>
#include <cstdio>
int main(){
 roulette::Game g;
 assert(g.players[0].name=="Alex"&&g.player_count==4);
 assert(g.all_ready());g.players[2].ready=false;g.start_session();assert(g.screen==roulette::Screen::Players);
 g.players[2].ready=true;g.seed(123456);g.start_session();assert(g.screen==roulette::Screen::Game);
 assert(g.slice_at(0)==0);assert(g.slice_at(roulette::tau-0.01f)==0);
 for(int i=0;i<100;i++){
  g.start_spin();assert(g.spinning);
  for(int j=0;j<600&&g.spinning;j++)g.update(1.f/60);
  assert(!g.spinning&&g.screen==roulette::Screen::Result);
  assert(g.winning_slice==g.pending_slice);
  assert(roulette::prompts[g.prompt_index].category==roulette::slices[g.winning_slice].category);
  g.next_turn();assert(g.screen==roulette::Screen::Game);
 }
 assert(g.turn==100);
 g.players[0].name="Sky";g.cycle_name(0);assert(g.players[0].name=="Robin");
 puts("PASS: 100 spins, category selection, turn rotation, ready validation, name cycling");
}
