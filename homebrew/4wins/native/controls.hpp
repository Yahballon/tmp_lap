// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "game.hpp"
#include <algorithm>
namespace wins4 {
struct Input {
 bool up=false,down=false,left=false,right=false,confirm=false,back=false,triangle=false,options=false,l1=false,r1=false;
};
struct Controller {
 bool bot_pending=false;
 unsigned long long bot_due=0;
 bool save_needed=false;
 int last_drop_col=-1,last_drop_row=-1,last_drop_owner=0;
 unsigned long long drop_time=0;
 void record_drop(Game& g,unsigned long long now){last_drop_col=g.last_drop_col;last_drop_row=g.last_drop_row;last_drop_owner=g.last_drop_owner;drop_time=now;save_needed=g.screen==Screen::Result;if(g.screen==Screen::Result)g.submenu=0;}
 void start(Game& g,unsigned long long now){g.reset();bot_pending=false;last_drop_col=-1;last_drop_row=-1;last_drop_owner=0;drop_time=now;}
 // Return true only when the app requested an exit. No state is changed after it.
 bool handle(Game& g,const Input& i,unsigned long long now){
  auto nav=[&](int count){if(i.up)g.submenu=(g.submenu+count-1)%count;if(i.down)g.submenu=(g.submenu+1)%count;};
  switch(g.screen){
   case Screen::Home:
    if(i.up)g.focus=(g.focus+3)%4;
    if(i.down)g.focus=(g.focus+1)%4;
    if(i.confirm){if(g.focus==0)start(g,now);else if(g.focus==1){g.screen=Screen::Mode;g.submenu=int(g.opponent);}else if(g.focus==2){g.screen=Screen::Settings;g.submenu=0;}else{g.screen=Screen::Exit;g.submenu=1;}}
    else if(i.back){g.screen=Screen::Exit;g.submenu=1;}
    break;
   case Screen::Mode:
    nav(2);
    if(i.confirm){g.opponent=Opponent(g.submenu);save_needed=true;if(g.opponent==Opponent::Bot){g.screen=Screen::Difficulty;g.submenu=int(g.level);}else{g.screen=Screen::Home;g.focus=1;}}
    if(i.back){g.screen=Screen::Home;g.focus=1;}break;
   case Screen::Difficulty:
    nav(3);if(i.confirm){g.level=Level(g.submenu);save_needed=true;g.screen=Screen::Home;g.focus=1;}
    if(i.back){g.screen=Screen::Mode;g.submenu=1;}break;
   case Screen::Settings:
    nav(4);
    if(i.confirm||i.left||i.right){
     if(g.submenu==0)g.first_player_red=!g.first_player_red;
     else if(g.submenu==1)g.level=Level((int(g.level)+(i.left?2:1))%3);
     else if(g.submenu==2)g.sound=!g.sound;
     else if(g.submenu==3){g.wins[0]=g.wins[1]=g.draws=0;}
     save_needed=true;
    }
    if(i.back){g.screen=Screen::Home;g.focus=2;}break;
   case Screen::Playing:
    if(i.options||i.back){g.screen=Screen::Paused;g.submenu=0;break;}
    if(g.opponent==Opponent::Bot&&g.turn==2){
     if(!bot_pending){bot_pending=true;bot_due=now+430000ULL;}
     if(now>=bot_due){int c=g.choose_bot();bot_pending=false;if(c>=0){g.selected=c;if(g.drop(c))record_drop(g,now);}}
     break;
    }
    if(i.left)g.selected=(g.selected+6)%7;
    if(i.right)g.selected=(g.selected+1)%7;
    if(i.l1)g.selected=0;
    if(i.r1)g.selected=6;
    if(i.confirm){if(g.drop(g.selected)){bot_pending=false;record_drop(g,now);}}
    break;
   case Screen::Paused:
    nav(3);
    if(i.confirm){if(g.submenu==0)g.screen=Screen::Playing;else if(g.submenu==1)start(g,now);else{g.screen=Screen::Home;g.focus=0;bot_pending=false;}}
    if(i.options||i.back)g.screen=Screen::Playing;
    break;
   case Screen::Result:
    nav(2);
    if(i.confirm){if(g.submenu==0)start(g,now);else{g.screen=Screen::Home;g.focus=0;}}
    if(i.back){g.screen=Screen::Home;g.focus=0;}break;
   case Screen::Exit:
    nav(2);
    if(i.back){g.screen=Screen::Home;g.focus=3;}
    else if(i.confirm){if(g.submenu==0)return true;g.screen=Screen::Home;g.focus=3;}
    break;
  }
  if(g.screen==Screen::Result&&g.submenu>1)g.submenu=0;
  return false;
 }
};
}
