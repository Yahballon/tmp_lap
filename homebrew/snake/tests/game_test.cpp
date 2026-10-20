#include "../native/game.hpp"
#include <cassert>
#include <cstdio>
using namespace snake;
int main(){
 Game g; assert(g.body.size()==12&&(g.food==Point{18,8}));
 assert(g.page==Page::Home);
 g.start();assert(g.page==Page::Game&&g.score==0);
 g.turn(Dir::Left);assert(g.queued==Dir::Right); // No immediate reverse
 g.turn(Dir::Up);assert(g.queued==Dir::Up);
 g.turn(Dir::Down);assert(g.queued==Dir::Up);
 g.step();assert((g.body.front()==Point{15,11}));
 g.turn(Dir::Right);g.step();assert((g.body.front()==Point{16,11}));
 g.food={17,11};g.step();assert(g.score==10&&g.apples==1&&g.body.size()==13&&g.high_score==10);
 g.toggle_pause();const auto old=g.body.front();g.tick(1);assert(g.body.front()==old);
 g.toggle_pause();
 g.start();g.food={16,12};g.set_food(2);g.step();assert(g.score==30);
 g.start();g.set_food(1);g.food={16,12};g.step();assert(g.score==15);
 // Wall death.
 g.start();g.walls=true;for(int i=0;i<20&&g.page==Page::Game;i++)g.step();
 assert(g.page==Page::GameOver);
 // Wraparound in no-walls mode.
 g.start();g.walls=false;g.mode=Mode::Zen;
 for(int i=0;i<12;i++)g.step();
 assert(g.body.front().x==3&&g.page==Page::Game);
 // Legal move into the moving tail.
 g.start();g.body={{2,2},{2,3},{1,3},{1,2}};g.direction=g.queued=Dir::Left;
 g.food={14,14};g.step();assert(g.page==Page::Game&&(g.body.front()==Point{1,2}));
 // Solid self-collision kills.
 g.start();g.mode=Mode::Classic;g.walls=true;
 g.body={{4,4},{4,5},{5,5},{5,4},{6,4}};g.direction=g.queued=Dir::Right;
 g.food={19,19};g.step();assert(g.page==Page::GameOver);
 // Food never spawns on snake, and speed/clamping are bounded.
 g.start();g.seed(33);for(int i=0;i<200;i++){assert(g.spawn());assert(!g.occupied(g.food));}
 g.set_speed(999);assert(g.speed==8&&g.interval()>=.075f);
 g.set_speed(-5);assert(g.speed==1);
 puts("PASS: Snake movement, food, collision, wrap, pause, score, speed, spawn");
}
