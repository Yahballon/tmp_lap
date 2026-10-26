#include "../native/workout.hpp"
#include <cassert>
#include <cstdio>
using namespace fitbrew;
int main(){
 static_assert(EXERCISE_COUNT>=30);
 static_assert(PLAN_COUNT>=10);
 for(const auto& p:plans){
  assert(p.count>0&&p.count<=10&&p.seconds>=20);
  for(int i=0;i<p.count;i++)assert(p.sequence[i]<EXERCISE_COUNT);
  assert(duration_seconds(p)>0);
 }
 State g;
 assert(g.screen==Screen::Home);
 g.handle({.right=true});assert(g.focus==1);
 g.handle({.confirm=true});assert(g.screen==Screen::Detail&&g.selected==1);
 g.handle({.confirm=true});assert(g.screen==Screen::Session&&g.remaining_ms>0);
 const int initial=g.remaining_ms;
 g.tick(240);assert(g.remaining_ms==initial-240);
 g.handle({.confirm=true});g.tick(240);assert(g.remaining_ms==initial-240);
 g.handle({.confirm=true});
 for(int i=0;i<3000&&g.screen==Screen::Session;i++)g.tick(250);
 assert(g.screen==Screen::Completed&&g.history.completed==1);
 assert(g.history.seconds==duration_seconds(plans[1]));
 assert(g.history.streak==1&&g.dirty);
 g.handle({.confirm=true});assert(g.screen==Screen::Home);
 g.handle({.back=true});assert(g.screen==Screen::Exit);
 g.handle({.left=true});assert(g.focus==0);
 assert(g.handle({.confirm=true}));
 puts("PASS FITBREW: exercise IDs, timer, rest, pause, completion, history, exit");
}
