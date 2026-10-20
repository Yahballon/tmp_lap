// SPDX-License-Identifier: GPL-3.0-or-later
// SNAKE by YAHBALLON -- portable, deterministic gameplay.
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

namespace snake {
constexpr int WIDTH=24, HEIGHT=24;
struct Point { int x=0,y=0; bool operator==(const Point& b) const {return x==b.x&&y==b.y;} };
enum class Dir { Up, Right, Down, Left };
enum class Page { Home, Game, Paused, Modes, Settings, Scores, GameOver, Exit };
enum class Mode { Classic, Zen, Sprint };
struct Game {
 Page page=Page::Home;
 Mode mode=Mode::Classic;
 Dir direction=Dir::Right,queued=Dir::Right;
 std::vector<Point> body;
 Point food{18,8};
 uint64_t rng=0x2545f4914f6cdd1dULL;
 int score=0,high_score=0,apples=0,focus=0,settings_focus=0,mode_focus=0;
 int speed=3,food_type=0; // Apple, Cherry, Golden
 bool walls=true,won=false,new_record=false,changed=false;
 float accumulator=0.f;
 Game(){reset_board();}
 void seed(uint64_t value){rng=value?value:0x2545f4914f6cdd1dULL;}
 uint32_t random(){uint64_t x=rng;x^=x>>12;x^=x<<25;x^=x>>27;rng=x;return uint32_t((x*2685821657736338717ULL)>>32);}
 static bool opposite(Dir a,Dir b){return (int(a)+2)%4==int(b);}
 bool occupied(Point p) const {return std::find(body.begin(),body.end(),p)!=body.end();}
 void reset_board(){
  body.clear();
  for(int x=15;x>=7;--x)body.push_back({x,12});
  body.push_back({7,11});body.push_back({7,10});body.push_back({7,9});
  direction=queued=Dir::Right;food={18,8};score=0;apples=0;won=false;new_record=false;
  accumulator=0.f;changed=false;
 }
 void start(){reset_board();page=Page::Game;}
 void turn(Dir d){if(page==Page::Game&&!opposite(direction,d)&&!opposite(queued,d))queued=d;}
 float interval() const {
  const int effective=std::clamp(speed+(mode==Mode::Sprint?3:0),1,11);
  return std::max(.075f,.37f-float(effective-1)*.035f);
 }
 bool spawn(){
  const int free=WIDTH*HEIGHT-int(body.size());
  if(free<=0)return false;
  int n=int(random()%uint32_t(free));
  for(int y=0;y<HEIGHT;y++)for(int x=0;x<WIDTH;x++){
   Point p{x,y};if(occupied(p))continue;
   if(n--==0){food=p;return true;}
  }
  return false;
 }
 bool step(){
  if(page!=Page::Game)return false;
  direction=queued;
  Point head=body.front();
  switch(direction){case Dir::Up:--head.y;break;case Dir::Right:++head.x;break;
   case Dir::Down:++head.y;break;case Dir::Left:--head.x;break;}
  const bool solid=walls&&mode!=Mode::Zen;
  if(solid&&(head.x<0||head.x>=WIDTH||head.y<0||head.y>=HEIGHT)){
   page=Page::GameOver;update_record();return false;
  }
  head.x=(head.x+WIDTH)%WIDTH;head.y=(head.y+HEIGHT)%HEIGHT;
  const bool ate=head==food;
  // Entering the old tail position is legal when the tail moves this tick.
  const size_t collision_length=body.size()-(ate?0:1);
  if(mode!=Mode::Zen&&std::find(body.begin(),body.begin()+collision_length,head)!=body.begin()+collision_length){
   page=Page::GameOver;update_record();return false;
  }
  body.insert(body.begin(),head);
  if(!ate)body.pop_back();
  else {
   score+=(food_type==2?30:food_type==1?15:10);
   ++apples;update_record();changed=true;
   if(!spawn()){won=true;page=Page::GameOver;}
  }
  return true;
 }
 void update_record(){if(score>high_score){high_score=score;new_record=true;changed=true;}}
 void tick(float elapsed){
  if(page!=Page::Game)return;
  accumulator+=std::clamp(elapsed,0.f,.1f);
  for(int i=0;i<3&&accumulator>=interval()&&page==Page::Game;i++){
   accumulator-=interval();step();
  }
 }
 void toggle_pause(){if(page==Page::Game)page=Page::Paused;else if(page==Page::Paused)page=Page::Game;}
 void set_speed(int s){speed=std::clamp(s,1,8);}
 void set_food(int f){food_type=(f%3+3)%3;}
 void set_mode(int m){mode=Mode((m%3+3)%3);}
};
} // namespace snake
