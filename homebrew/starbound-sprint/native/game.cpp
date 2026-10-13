// SPDX-License-Identifier: GPL-3.0-or-later
#include "game.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>

namespace sprint {
namespace {
constexpr float kPlayerWidth=42.f, kPlayerHeight=54.f;
constexpr char kProgressFile[]="/data/homebrew/PPSA99641/starbound-progress.cfg";
constexpr char kProgressTemp[]="/data/homebrew/PPSA99641/starbound-progress.tmp";
constexpr const char* kNames[kStages] = {
    "MOONLIT MEADOW", "CRYSTAL RIFT", "CLOUD CIRCUIT", "EMBER GARDEN",
    "FROST ENGINE", "VIOLET VOID", "SOLAR FOUNDRY", "STARLIGHT SUMMIT"
};
bool overlap(float ax,float ay,float aw,float ah,float bx,float by,float bw,float bh) {
    return ax < bx+bw && ax+aw > bx && ay < by+bh && ay+ah > by;
}
float clampf(float n,float a,float b){return std::max(a,std::min(b,n));}
}
const char* Game::stage_name(int i){return kNames[std::max(0,std::min(kStages-1,i))];}
void Progress::load() {
    FILE* f=std::fopen(kProgressFile,"rb");
    if(!f)return;
    int version=0, u=0, b=0, m=0;
    bool ok=std::fscanf(f,"STARBOUND %d %d %d %d",&version,&u,&b,&m)==4;
    std::fclose(f);
    if(ok && version==1 && u>=0 && u<kStages && b>=0 && b<100000000) {
        unlocked=u; best_score=b; reduced_motion=m!=0;
    }
}
bool Progress::save() const {
    mkdir("/data/homebrew/PPSA99641",0755);
    FILE* f=std::fopen(kProgressTemp,"wb");
    if(!f)return false;
    int count=std::fprintf(f,"STARBOUND 1 %d %d %d\n",unlocked,best_score,reduced_motion?1:0);
    if(count<=0 || std::fflush(f)!=0 || std::fclose(f)!=0) {
        std::remove(kProgressTemp);return false;
    }
    if(std::rename(kProgressTemp,kProgressFile)!=0) {
        std::remove(kProgressTemp);return false;
    }
    return true;
}
Game::Game() { progress.load(); make_stage(); }
bool Game::solid_at(int col,int row) const {
    if(col<0 || col>=kColumns)return true;
    if(row<0 || row>=kRows)return false;
    return tiles[row][col]=='#'||tiles[row][col]=='=';
}
void Game::make_stage() {
    pickups.clear();drones.clear();
    for(auto& row:tiles)row.fill('.');
    // Every stage has its own gap rhythm; no maps are copied from another game.
    const int shift=stage%4;
    for(int c=0;c<kColumns;c++)tiles[kRows-1][c]='#';
    const int holes[4]={21+shift,43-shift,70+shift,94-shift};
    for(int i=0;i<4;i++) {
        int size=(i%2==0)?2:3;
        for(int c=holes[i];c<holes[i]+size;c++)tiles[kRows-1][c]='.';
    }
    for(int i=0;i<13;i++){
        int column=9+i*8+(stage*3+i)%4;
        int row= (i%3==0)?7:((i%3==1)?9:8);
        int length=3+(i+stage)%4;
        for(int x=column;x<std::min(kColumns-7,column+length);x++)tiles[row][x]='=';
        pickups.push_back({{(column+1.f)*kTile,(row-1.f)*kTile+6},false});
        if(i%3==0)pickups.push_back({{(column+2.f)*kTile,(row-2.f)*kTile+3},false});
    }
    // Low crystals and airborne gems guide the player over original obstacles.
    for(int c=5;c<kColumns-7;c+=5){
        bool safe=tiles[kRows-1][c]=='#';
        if(safe)pickups.push_back({{c*kTile+16.f,11*kTile+8.f},false});
    }
    for(int i=0;i<10;i++){
        int col=15+i*10+(stage+i)%3;
        if(tiles[kRows-1][col]=='#')drones.push_back({{col*kTile+10.f,12*kTile-42.f},i%2?1.f:-1.f,true});
    }
    for(int c=36;c<kColumns-15;c+=32){
        if(tiles[kRows-1][c]=='#')tiles[11][c]='S'; // launch pads, nonblocking
    }
    checkpoint_x=2.f*kTile;
    player={checkpoint_x,12.f*kTile-kPlayerHeight};
    velocity={};grounded=false;invulnerable=0;elapsed=0;camera=0;
}
void Game::new_run(int first_stage) {
    stage=std::max(0,std::min(progress.unlocked,first_stage));
    lives=3;score=0;crystals=0;phase=Phase::playing;make_stage();
}
void Game::next_stage(){
    if(phase!=Phase::stage_clear)return;
    if(stage+1>=kStages){phase=Phase::victory;return;}
    stage++;make_stage();phase=Phase::playing;
}
void Game::reset_to_menu(){phase=Phase::menu;}
void Game::set_pause(bool paused){
    if(paused&&phase==Phase::playing)phase=Phase::paused;
    if(!paused&&phase==Phase::paused)phase=Phase::playing;
}
void Game::respawn(){
    player={checkpoint_x,12.f*kTile-kPlayerHeight};
    velocity={};invulnerable=110;grounded=false;
}
void Game::damage(){
    if(invulnerable>0||phase!=Phase::playing)return;
    if(--lives<=0){phase=Phase::game_over;progress.best_score=std::max(progress.best_score,score);progress.save();}
    else respawn();
}
void Game::move_x(float dx) {
    float goal=player.x+dx;
    if(dx==0)return;
    if(dx>0){
        int right=static_cast<int>(std::floor((goal+kPlayerWidth-0.01f)/kTile));
        int top=static_cast<int>(std::floor((player.y+2.f)/kTile));
        int bottom=static_cast<int>(std::floor((player.y+kPlayerHeight-2.f)/kTile));
        for(int r=top;r<=bottom;r++)if(solid_at(right,r)){goal=std::min(goal,right*kTile-kPlayerWidth);velocity.x=0;break;}
    }else {
        int left=static_cast<int>(std::floor(goal/kTile));
        int top=static_cast<int>(std::floor((player.y+2.f)/kTile));
        int bottom=static_cast<int>(std::floor((player.y+kPlayerHeight-2.f)/kTile));
        for(int r=top;r<=bottom;r++)if(solid_at(left,r)){goal=std::max(goal,(left+1)*kTile*1.f);velocity.x=0;break;}
    }
    player.x=clampf(goal,0.f,static_cast<float>((kColumns-1)*kTile));
}
void Game::move_y(float dy) {
    float goal=player.y+dy;grounded=false;
    if(dy>0){
        int bottom=static_cast<int>(std::floor((goal+kPlayerHeight)/kTile));
        int left=static_cast<int>(std::floor((player.x+2.f)/kTile));
        int right=static_cast<int>(std::floor((player.x+kPlayerWidth-2.f)/kTile));
        for(int col=left;col<=right;col++)if(solid_at(col,bottom)) {
            goal=std::min(goal,bottom*kTile-kPlayerHeight);velocity.y=0;grounded=true;break;
        }
    }else if(dy<0){
        int top=static_cast<int>(std::floor(goal/kTile));
        int left=static_cast<int>(std::floor((player.x+2.f)/kTile));
        int right=static_cast<int>(std::floor((player.x+kPlayerWidth-2.f)/kTile));
        for(int col=left;col<=right;col++)if(solid_at(col,top)) {
            goal=std::max(goal,(top+1)*kTile*1.f);velocity.y=0;break;
        }
    }
    player.y=goal;
}
void Game::step(const Control& input) {
    if(phase!=Phase::playing)return;
    elapsed+=1.f/60.f;
    if(invulnerable>0)--invulnerable;
    float target=clampf(input.horizontal,-1.f,1.f)*(input.sprint?7.4f:5.2f);
    velocity.x+=(target-velocity.x)*0.24f;
    if(std::fabs(input.horizontal)>.1f)facing_right=input.horizontal>0;
    bool triggered=input.jump&&!jump_was_held;
    jump_was_held=input.jump;
    if(triggered&&grounded){velocity.y=-15.4f;grounded=false;}
    // Slightly shorter jump on early release, plus gravity.
    if(!input.jump&&velocity.y<-6.f)velocity.y+=0.43f;
    velocity.y=std::min(17.f,velocity.y+0.68f);
    move_x(velocity.x);
    float beforeBottom=player.y+kPlayerHeight;
    move_y(velocity.y);
    for(int c=std::max(0,static_cast<int>(player.x/kTile)-1);
        c<=std::min(kColumns-1,static_cast<int>((player.x+kPlayerWidth)/kTile)+1);c++) {
        if(tiles[11][c]=='S' && velocity.y>=0 &&
           beforeBottom<=11.f*kTile+20 &&
           player.y+kPlayerHeight>=11.f*kTile &&
           overlap(player.x,player.y,kPlayerWidth,kPlayerHeight,
                   c*kTile*1.f,11.f*kTile,1.f*kTile,22.f)){
            velocity.y=-20.5f;grounded=false;
        }
    }
    if(player.y>kRows*kTile+140.f){damage();return;}
    if(player.x>checkpoint_x+31.f*kTile)checkpoint_x=std::min(player.x,90.f*kTile);
    for(auto& p:pickups)if(!p.taken && overlap(player.x,player.y,kPlayerWidth,kPlayerHeight,p.pos.x,p.pos.y,28.f,32.f)) {
        p.taken=true;crystals++;score+=100;
    }
    for(auto& d:drones)if(d.alive){
        float ahead=d.pos.x+d.direction*1.45f;
        int floorcol=static_cast<int>((ahead+(d.direction>0?35.f:0.f))/kTile);
        int floorrow=12;
        if(!solid_at(floorcol,floorrow) || ahead<5*kTile || ahead>(kColumns-8)*kTile) d.direction=-d.direction;
        else d.pos.x=ahead;
        if(overlap(player.x,player.y,kPlayerWidth,kPlayerHeight,d.pos.x,d.pos.y,42.f,40.f)){
            if(velocity.y>1.f && beforeBottom<=d.pos.y+13.f){
                d.alive=false;velocity.y=-10.8f;score+=250;grounded=false;
            }else{damage();if(phase!=Phase::playing)return;}
        }
    }
    camera=clampf(player.x-570.f,0.f,kColumns*kTile-1760.f);
    if(player.x>=finish_x()){
        phase=Phase::stage_clear;score+=1000;
        if(stage+1<kStages)progress.unlocked=std::max(progress.unlocked,stage+1);
        progress.best_score=std::max(progress.best_score,score);progress.save();
    }
}
} // namespace sprint
