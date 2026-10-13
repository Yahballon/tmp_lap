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
    "AURORA GROVE", "PRISM CATACOMBS", "SKYDOCK CIRCUIT", "EMBER ORCHARD",
    "GLACIER RELAY", "NEBULA SANCTUM", "SOLAR MACHINERY", "LAST STAR GATE"
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
// This CSV importer is an independent native PS5 implementation of the
// Tiled-style data-driven maps in Gold872/Super-Mario-Bros (MIT).
// Do not import any Nintendo level coordinates or proprietary media.
static bool load_original_csv(int stage,
    std::array<std::array<char,kColumns>,kRows>& tiles,
    std::vector<Pickup>& pickups,
    std::vector<Drone>& drones,
    std::vector<MovingPlatform>& platforms) {
    char path[160];
    std::snprintf(path,sizeof(path),"/app0/assets/levels/world_%02d.csv",stage+1);
    FILE* file=std::fopen(path,"rb");
    if(!file){
        std::snprintf(path,sizeof(path),"/data/homebrew/PPSA99641/assets/levels/world_%02d.csv",stage+1);
        file=std::fopen(path,"rb");
    }
    if(!file){
        std::snprintf(path,sizeof(path),"homebrew/starbound-sprint/levels/world_%02d.csv",stage+1);
        file=std::fopen(path,"rb");
    }
    if(!file)return false;
    std::array<std::array<int,kColumns>,kRows> data{};
    bool ok=true;char line[1200];
    for(int row=0;row<kRows && ok;++row){
        if(!std::fgets(line,sizeof(line),file)){ok=false;break;}
        char* pos=line;
        for(int col=0;col<kColumns;++col){
            char* next=nullptr;
            long id=std::strtol(pos,&next,10);
            if(next==pos || id<0 || id>8){ok=false;break;}
            data[row][col]=(int)id;
            pos=next;
            if(col<kColumns-1){
                if(*pos!=','){ok=false;break;}
                ++pos;
            }else if(*pos!='\n' && *pos!='\r' && *pos!='\0')ok=false;
        }
    }
    if(std::fgets(line,sizeof(line),file))ok=false;
    std::fclose(file);
    if(!ok)return false;
    for(auto& row:tiles)row.fill('.');
    for(int row=0;row<kRows;++row)for(int col=0;col<kColumns;++col){
        const int id=data[row][col];
        const float x=col*kTile*1.f,y=row*kTile*1.f;
        switch(id){
        case 1:tiles[row][col]='#';break;
        case 2:tiles[row][col]='=';break;
        case 3:pickups.push_back({{x+14.f,y+12.f},false});break;
        case 4:drones.push_back({{x+9.f,y+17.f},col%2?1.f:-1.f,true,stage%8,y+17.f});break;
        case 5:tiles[row][col]='S';break;
        case 6:tiles[row][col]='^';break;
        case 7:drones.push_back({{x+9.f,y-125.f},col%2?1.f:-1.f,true,3,y-125.f});break;
        case 8:platforms.push_back({{x,y},x,(col%5)*.7f,80.f});break;
        default:break;
        }
    }
    return true;
}

void Game::make_stage() {
    pickups.clear();drones.clear();moving_platforms.clear();
    for(auto& row:tiles)row.fill('.');
    // Gold872's level architecture uses Tiled CSV + entity layers.
    // Our eight files are freshly authored, with original level geometry.
    if(load_original_csv(stage,tiles,pickups,drones,moving_platforms)){
        checkpoint_x=2.f*kTile;
        player={checkpoint_x,12.f*kTile-kPlayerHeight};
        velocity={};grounded=true;jumps_used_=0;jump_was_held=false;
        invulnerable=0;elapsed=0;camera=0;
        return;
    }

    // Eight authored world configurations: different rhythms, obstacles and height profiles.
    // Collisions are always queried from the tile map; decoration is render-only.
    const int theme=stage%4;
    for(int x=0;x<kColumns;++x)tiles[kRows-1][x]='#';
    for(int sector=0;sector<6;++sector){
        const int pit=23+sector*27+(stage+sector*3)%6;
        const int width=2+(stage+sector)%3;
        for(int c=pit;c<pit+width;++c)tiles[kRows-1][c]='.';
        // A logical route above every dangerous gap, with two possible jump arcs.
        const int bridge_row=7+((sector+stage)%3);
        for(int c=pit-3;c<pit+width+3;++c)if(c>=1&&c<kColumns-6)
            tiles[bridge_row][c]='=';
    }
    for(int part=0;part<20;++part){
        const int begin=7+part*8+(part*3+stage)%3;
        const int row=6+((part*7+stage*3)%5);
        const int length=2+((part+stage)%4);
        for(int c=begin;c<begin+length && c<kColumns-4;++c)
            if(tiles[row][c]=='.')tiles[row][c]='=';
        pickups.push_back({{(begin+1.f)*kTile,(row-1.f)*kTile+10.f},false});
        if(part%2==0)
            pickups.push_back({{(begin+2.f)*kTile,(row-2.f)*kTile+8.f},false});
    }
    // Crystal paths guide exploration and teach timing around hazards.
    for(int col=5;col<kColumns-6;col+=4){
        if(tiles[kRows-1][col]=='#'){
            int elevated=(col/4+stage)%4==0;
            pickups.push_back({{col*kTile+14.f,(elevated?9.f:11.f)*kTile+9.f},false});
        }
    }
    for(int i=0;i<16;++i){
        const int col=13+i*10+(stage+i*2)%4;
        const int type=(i+stage*2)%8;
        const bool flying=(type==3||type==4||type==7);
        if(flying||tiles[kRows-1][col]=='#'){
            float y=flying?9.4f*kTile:12.f*kTile-42.f;
            drones.push_back({{col*kTile+10.f,y},i%2?1.f:-1.f,true,type,y});
        }
    }
    for(int i=0;i<4;++i){
        int col=29+i*38+(stage*3)%8;
        if(tiles[kRows-1][col]=='#')tiles[11][col]='S';
    }
    for(int i=0;i<7;++i){
        int col=18+i*22+(stage*2+i)%4;
        if(col<kColumns-5 && tiles[kRows-1][col]=='#' &&
           tiles[11][col]=='.' && tiles[10][col]=='.'){
            tiles[11][col]='^';
        }
    }
    if(theme==1||theme==3)for(int i=0;i<4;++i){
        float x=(35+i*34)*kTile*1.f;
        float y=(7+(i+stage)%3)*kTile*1.f;
        moving_platforms.push_back({{x,y},x,i*1.3f,90.f+(i%2)*30.f});
    }
    checkpoint_x=2.f*kTile;
    player={checkpoint_x,12.f*kTile-kPlayerHeight};
    velocity={};grounded=true;jumps_used_=0;jump_was_held=false;invulnerable=0;elapsed=0;camera=0;
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
    velocity={};invulnerable=110;grounded=true;jumps_used_=0;jump_was_held=false;
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
    // Moving platforms carry the player and provide actual one-way collision.
    for(auto& p:moving_platforms){
        const float last=p.pos.x;
        p.pos.x=p.anchor+std::sin(elapsed*1.3f+p.phase)*p.range;
        const bool riding=std::fabs(player.y+kPlayerHeight-p.pos.y)<3.f &&
            player.x+kPlayerWidth>last && player.x<last+3.f*kTile && grounded;
        if(riding)player.x+=p.pos.x-last;
    }
    float target=clampf(input.horizontal,-1.f,1.f)*(input.sprint?7.4f:5.2f);
    velocity.x+=(target-velocity.x)*0.24f;
    if(std::fabs(input.horizontal)>.1f)facing_right=input.horizontal>0;
    bool triggered=input.jump&&!jump_was_held;
    jump_was_held=input.jump;
    // Cross must be released between jumps; at most two launches per landing.
    if(grounded)jumps_used_=0;
    if(triggered&&jumps_used_<2){
        velocity.y=jumps_used_==0?-15.4f:-13.2f;
        ++jumps_used_;
        grounded=false;
    }
    // Slightly shorter jump on early release, plus gravity.
    if(!input.jump&&velocity.y<-6.f)velocity.y+=0.43f;
    velocity.y=std::min(17.f,velocity.y+0.68f);
    move_x(velocity.x);
    float beforeBottom=player.y+kPlayerHeight;
    move_y(velocity.y);
    if(velocity.y>=0.f){
        for(auto& p:moving_platforms){
            if(beforeBottom<=p.pos.y+8.f && player.y+kPlayerHeight>=p.pos.y &&
               player.x+kPlayerWidth>p.pos.x+4.f && player.x<p.pos.x+3.f*kTile-4.f){
                player.y=p.pos.y-kPlayerHeight;
                velocity.y=0;grounded=true;break;
            }
        }
    }
    if(grounded)jumps_used_=0; // Restore both jumps on ground or a platform.
    for(int c=std::max(0,static_cast<int>(player.x/kTile)-1);
        c<=std::min(kColumns-1,static_cast<int>((player.x+kPlayerWidth)/kTile)+1);c++) {
        if(tiles[11][c]=='S' && velocity.y>=0 &&
           beforeBottom<=11.f*kTile+20 &&
           player.y+kPlayerHeight>=11.f*kTile &&
           overlap(player.x,player.y,kPlayerWidth,kPlayerHeight,
                   c*kTile*1.f,11.f*kTile,1.f*kTile,22.f)){
            velocity.y=-20.5f;grounded=false;jumps_used_=1; // One midair jump after a launch pad.
        }
    }
    int leftHazard=std::max(0,(int)(player.x/kTile));
    int rightHazard=std::min(kColumns-1,(int)((player.x+kPlayerWidth-1.f)/kTile));
    for(int col=leftHazard;col<=rightHazard;++col){
        if(tiles[11][col]=='^' &&
           overlap(player.x,player.y,kPlayerWidth,kPlayerHeight,
                   col*kTile*1.f,11.f*kTile+13.f,kTile*1.f,kTile-13.f)){
            damage();return;
        }
    }
    if(player.y>kRows*kTile+140.f){damage();return;}
    if(player.x>checkpoint_x+31.f*kTile)checkpoint_x=std::min(player.x,90.f*kTile);
    for(auto& p:pickups)if(!p.taken && overlap(player.x,player.y,kPlayerWidth,kPlayerHeight,p.pos.x,p.pos.y,28.f,32.f)) {
        p.taken=true;crystals++;score+=100;
    }
    for(auto& d:drones)if(d.alive){
        float ahead=d.pos.x+d.direction*(d.type==3||d.type==7?2.1f:1.45f);
        int floorcol=static_cast<int>((ahead+(d.direction>0?35.f:0.f))/kTile);
        int floorrow=12;
        const bool airborne=d.type==3||d.type==4||d.type==7;
        if((!airborne&&!solid_at(floorcol,floorrow)) || ahead<5*kTile || ahead>(kColumns-8)*kTile) d.direction=-d.direction;
        else d.pos.x=ahead;
        d.pos.y=airborne?d.home_y+std::sin(elapsed*3.f+d.pos.x*.018f)*22.f:d.home_y;
        if(overlap(player.x,player.y,kPlayerWidth,kPlayerHeight,d.pos.x,d.pos.y,42.f,40.f)){
            if(velocity.y>1.f && beforeBottom<=d.pos.y+13.f){
                d.alive=false;velocity.y=-10.8f;score+=250;grounded=false;jumps_used_=1; // Stomp bounce restores an air jump.
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
