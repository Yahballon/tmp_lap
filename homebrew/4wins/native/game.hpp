// SPDX-License-Identifier: GPL-3.0-or-later
// 4 WINS by YAHBALLON - deterministic Connect Four rules and offline AI.
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>

namespace wins4 {
constexpr int COLS=7, ROWS=6;
enum class Screen {Home, Mode, Difficulty, Settings, Playing, Paused, Result, Exit};
enum class Opponent { Human, Bot };
enum class Level { Easy, Medium, Hard };
struct Game {
  std::array<std::array<uint8_t,ROWS>,COLS> grid{}; // row 0 is bottom
  std::array<int,COLS> heights{};
  Screen screen=Screen::Home;
  Opponent opponent=Opponent::Human;
  Level level=Level::Medium;
  int focus=0,submenu=0,selected=3,turn=1,winner=0,moves=0;
  int last_drop_col=-1,last_drop_row=-1,last_drop_owner=0;
  int wins[2]{0,0}, draws=0;
  bool sound=true,first_player_red=true;
  uint64_t seed=0x5a795f1337badULL;
  int winning_cells[8]{-1,-1,-1,-1,-1,-1,-1,-1};
  uint32_t random(){ seed^=seed>>12;seed^=seed<<25;seed^=seed>>27;return uint32_t((seed*2685821657736338717ULL)>>32); }
  bool legal(int col)const{return col>=0&&col<COLS&&heights[col]<ROWS;}
  bool full()const{return moves==COLS*ROWS;}
  int at(int c,int r)const{return c>=0&&c<COLS&&r>=0&&r<ROWS?int(grid[c][r]):0;}
  void reset(){for(auto& c:grid)c.fill(0);heights.fill(0);selected=3;winner=0;moves=0;last_drop_col=-1;last_drop_row=-1;last_drop_owner=0;turn=first_player_red?1:2;winning_cells[0]=-1;screen=Screen::Playing;}
  void undo(int col){if(col<0||col>=COLS||heights[col]==0)return;--heights[col];grid[col][heights[col]]=0;--moves;winner=0;winning_cells[0]=-1;}
  bool makes_four(int c,int r,int player,bool record=false){
    constexpr int dirs[4][2]={{1,0},{0,1},{1,1},{1,-1}};
    for(const auto& d:dirs){int count=1;int start_c=c,start_r=r;
      for(int sign:{-1,1}){int x=c+sign*d[0],y=r+sign*d[1];while(at(x,y)==player){++count;if(sign==-1){start_c=x;start_r=y;}x+=sign*d[0];y+=sign*d[1];}}
      if(count>=4){if(record){for(int i=0;i<4;i++){winning_cells[i*2]=start_c+i*d[0];winning_cells[i*2+1]=start_r+i*d[1];}}return true;}
    }return false;
  }
  bool drop(int col){
    if(!legal(col)||winner!=0)return false;
    int r=heights[col]++;grid[col][r]=uint8_t(turn);last_drop_col=col;last_drop_row=r;last_drop_owner=turn;++moves;
    if(makes_four(col,r,turn,true)){winner=turn;wins[turn-1]++;screen=Screen::Result;}
    else if(full()){winner=3;++draws;screen=Screen::Result;}
    else turn=3-turn;
    return true;
  }
  bool search_drop(int col,int player){if(!legal(col))return false;int r=heights[col]++;grid[col][r]=uint8_t(player);++moves;return makes_four(col,r,player);}
  static constexpr int order[7]={3,2,4,1,5,0,6};
  int immediate_win(int player){for(int c:order)if(legal(c)){bool win=search_drop(c,player);undo(c);if(win)return c;}return -1;}
  int score_window(int a,int b,int c,int d,int side)const{
    int own=(a==side)+(b==side)+(c==side)+(d==side);
    int opp=(a==3-side)+(b==3-side)+(c==3-side)+(d==3-side);
    if(own&&opp)return 0;
    if(own==4)return 100000;
    if(opp==4)return -100000;
    static constexpr int scores[]={0,2,17,105,100000};
    static constexpr int blocks[]={0,2,20,135,100000};
    return own?scores[own]:opp?-blocks[opp]:0;
  }
  int evaluate(int side)const{
    int sum=0;
    for(int r=0;r<ROWS;r++){if(at(3,r)==side)sum+=8;else if(at(3,r)==3-side)sum-=8;}
    for(int r=0;r<ROWS;r++)for(int c=0;c<COLS;c++){
      if(c+3<COLS)sum+=score_window(at(c,r),at(c+1,r),at(c+2,r),at(c+3,r),side);
      if(r+3<ROWS)sum+=score_window(at(c,r),at(c,r+1),at(c,r+2),at(c,r+3),side);
      if(c+3<COLS&&r+3<ROWS)sum+=score_window(at(c,r),at(c+1,r+1),at(c+2,r+2),at(c+3,r+3),side);
      if(c+3<COLS&&r-3>=0)sum+=score_window(at(c,r),at(c+1,r-1),at(c+2,r-2),at(c+3,r-3),side);
    }
    return sum;
  }
  int nodes=0;
  int negamax(int depth,int alpha,int beta,int side){
    if(depth<=0||full()||nodes++>=80000)return evaluate(side);
    int best=-2000000;
    for(int col:order){if(!legal(col))continue;
      const bool win=search_drop(col,side);
      int value=win?100000+depth*100:-negamax(depth-1,-beta,-alpha,3-side);
      undo(col);best=std::max(best,value);alpha=std::max(alpha,value);
      if(alpha>=beta)break;
    }
    return best==-2000000?0:best;
  }
  int choose_bot(){
    if(full())return -1;
    // Every level can finish a win; Easy otherwise plays randomly.
    int c=immediate_win(2);if(c>=0)return c;
    if(level!=Level::Easy){c=immediate_win(1);if(c>=0)return c;}
    if(level==Level::Easy){int legal_cols[COLS],n=0;for(int x=0;x<COLS;x++)if(legal(x))legal_cols[n++]=x;return n?legal_cols[random()%unsigned(n)]:-1;}
    const int depth=level==Level::Hard?7:3;
    nodes=0;int best=-2000000,chosen=-1;
    for(int x:order){if(!legal(x))continue;
      bool win=search_drop(x,2);
      int value=win?1000000:-negamax(depth-1,-1900000,1900000,1);
      undo(x);
      // Medium intentionally varies similarly scored choices.
      if(level==Level::Medium)value+=int(random()%19)-9;
      if(value>best){best=value;chosen=x;}
    }
    return chosen;
  }
};
} // namespace wins4
