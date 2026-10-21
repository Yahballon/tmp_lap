// SPDX-License-Identifier: GPL-3.0-or-later
// Native ORIGINAL demo-1 enemies/spikes. Approximate source SurgeScripts;
// neither SurgeScript VM nor full original Enemy/Platformer behaviors are linked.
#pragma once
#include "gfx/renderer.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace surgepreview {
struct Hazard {
    enum Kind { Mosquito, Wolfey, Bugsy, Spikes } kind=Spikes;
    float origin_x=0, origin_y=0, x=0, y=0, elapsed=0, direction=-1;
    bool alive=true;
};
struct HazardBullet {float x=0,y=0,dir=0,age=0;};
struct HazardEvent {
    bool hit=false;
    bool lost_life=false;
    int killed=0;
    float source_x=0;
};
class LevelHazards {
public:
    bool load(hui::gfx::Renderer& renderer,const std::string& root) {
        ready_=false;error_.clear();hazards_.clear();bullets_.clear();
        if(root.size()<5||root.substr(root.size()-5)!="game/"){
            error_="HAZARD GAME ROOT";return false;
        }
        const std::string base=root.substr(0,root.size()-5)+"level-preview/";
        const std::string list=read(base+"hazards.tsv",8192);
        if(list.empty()){error_="HAZARDS MISSING";return false;}
        size_t start=0;
        while(start<list.size()){
            const auto end=list.find('\n',start);
            const auto row=list.substr(start,end==std::string::npos?
                                       std::string::npos:end-start);
            if(!row.empty()){
                char type[24]{};
                int x=0,y=0;
                if(std::sscanf(row.c_str(),"%23s %d %d",type,&x,&y)!=3 ||
                   std::abs(x)>100000||std::abs(y)>100000){
                    error_="BAD HAZARD POSITION";return false;
                }
                Hazard h;
                const std::string t(type);
                if(t=="mosquito")h.kind=Hazard::Mosquito;
                else if(t=="wolfey")h.kind=Hazard::Wolfey;
                else if(t=="bugsy")h.kind=Hazard::Bugsy;
                else if(t=="spikes")h.kind=Hazard::Spikes;
                else {error_="UNKNOWN HAZARD";return false;}
                h.origin_x=h.x=float(x);
                h.origin_y=h.y=float(y);
                h.direction=(h.kind==Hazard::Bugsy)?1.f:-1.f;
                hazards_.push_back(h);
            }
            start=end==std::string::npos?list.size():end+1;
        }
        int counts[4]{};
        for(const auto& h:hazards_)++counts[int(h.kind)];
        if(counts[0]!=5||counts[1]!=1||counts[2]!=1||counts[3]!=6){
            error_="ORIGINAL HAZARD COUNT";return false;
        }
        const char* labels[]={"mosquito","wolfey","bugsy","spikes"};
        for(int i=0;i<4;++i){
            if(!texture(renderer,base,labels[i],tex_[i],sheet_w_[i],sheet_h_[i])){
                error_="ORIGINAL HAZARD ART";return false;
            }
        }
        if(sheet_w_[0]<180||sheet_h_[0]<80||
           sheet_w_[1]<255||sheet_h_[1]<33||
           sheet_w_[2]<192||sheet_h_[2]<96||
           sheet_w_[3]<138||sheet_h_[3]<112){
            error_="ORIGINAL HAZARD ATLAS";return false;
        }
        ready_=true;reset_game();
        return true;
    }
    bool ready() const{return ready_;}
    const std::string& error() const{return error_;}
    size_t count() const{return hazards_.size();}
    int lives() const{return lives_;}
    bool game_over() const{return lives_<=0;}
    int score() const{return score_;}
    int defeated() const{return defeated_;}
    int hits() const{return hits_;}
    float invulnerability() const{return invuln_;}
    void restart_level() {
        bullets_.clear();invuln_=0;clock_=0;
        for(auto& h:hazards_){
            h.x=h.origin_x;h.y=h.origin_y;
            h.direction=h.kind==Hazard::Bugsy?1.f:-1.f;
            h.alive=true;h.elapsed=0;
        }
    }
    void reset_game(){
        lives_=3;score_=defeated_=hits_=0;restart_level();
    }
    HazardEvent tick(float dt,float px,float feet,float vy,bool attacking,int rings){
        HazardEvent event;
        if(!ready_)return event;
        dt=std::clamp(dt,0.f,1.f/60.f);
        clock_+=dt;
        invuln_=std::max(0.f,invuln_-dt);
        // Keep all combat simulation anchored to original entity positions.
        for(auto& h:hazards_){
            if(!h.alive)continue;
            const float last=h.elapsed;
            h.elapsed+=dt;
            if(h.kind==Hazard::Mosquito)
                h.x-=60.f*dt; // original DirectionalMovement left / speed=60
            else if(h.kind==Hazard::Wolfey || h.kind==Hazard::Bugsy){
                // The source Platformer() also respects walls and gaps; this
                // preliminary native patrol stays bounded around spawn.
                if(int(h.elapsed/2.5f)!=int(last/2.5f))
                    h.direction=-h.direction;
                const float speed=h.kind==Hazard::Wolfey?80.f:30.f;
                h.x=std::clamp(h.x+h.direction*speed*dt,
                               h.origin_x-110.f,h.origin_x+110.f);
                if(h.x==h.origin_x-110.f)h.direction=1.f;
                if(h.x==h.origin_x+110.f)h.direction=-1.f;
                if(h.kind==Hazard::Bugsy){
                    // Source Lady Bugsy fires volleys every ~2.5 seconds.
                    // A bounded approximation without the SurgeScript VM.
                    if(int(h.elapsed/2.5f)!=int(last/2.5f) && bullets_.size()<24)
                        bullets_.push_back({h.x+24.f*h.direction,h.y-18.f,h.direction,0.f});
                }
            }
            const bool is_spikes=h.kind==Hazard::Spikes;
            const float cy=(h.kind==Hazard::Mosquito)?h.y:
                           (h.kind==Hazard::Wolfey)?h.y-16.f:
                           (h.kind==Hazard::Bugsy)?h.y-16.f:h.y-10.f;
            const float width=is_spikes?15.f:24.f;
            const float height=is_spikes?10.f:26.f;
            if(std::abs(px-h.x)>width||std::abs((feet-20.f)-cy)>height+10.f)
                continue;
            if(!is_spikes && attacking){
                h.alive=false;++defeated_;score_+=100;++event.killed;
            }else if(invuln_<=0.f) {
                // Spike collisions only when player feet are near the spikes.
                if(is_spikes && (feet<h.y-28.f||feet>h.y+7.f))continue;
                event.hit=true;event.source_x=h.x;
                invuln_=1.6f;++hits_;
                if(rings==0) {
                    event.lost_life=true;
                    lives_=std::max(0,lives_-1);
                }
            }
        }
        for(auto& b:bullets_){
            b.x+=b.dir*60.f*dt;
            b.age+=dt;
            if(!event.hit&&invuln_<=0.f &&std::abs(px-b.x)<18.f&&
               std::abs(feet-20.f-b.y)<22.f){
                event.hit=true;event.source_x=b.x;
                invuln_=1.6f;++hits_;b.age=99.f;
                if(rings==0){event.lost_life=true;lives_=std::max(0,lives_-1);}
            }
        }
        bullets_.erase(std::remove_if(bullets_.begin(),bullets_.end(),
                    [](const HazardBullet& b){return b.age>5.f;}),bullets_.end());
        return event;
    }
    void draw(hui::gfx::DrawList& ui,float playerx,float feet){
        if(!ready_)return;
        const float cx=playerx-480.f,cy=feet-270.f;
        const hui::gfx::Color white=hui::gfx::Color::rgb(0xffffff);
        for(const auto& h:hazards_){
            if(!h.alive||h.x<cx-65.f||h.x>cx+1040.f||
               h.y<cy-90.f||h.y>cy+640.f)continue;
            int sx=0,sy=0,w=32,hh=32,hx=16,hy=32;
            if(h.kind==Hazard::Mosquito){
                w=36;hh=32;hx=18;hy=16;
                sx=(int(clock_*12.f)%2)*36;sy=16;
            }else if(h.kind==Hazard::Wolfey){
                w=51;hh=33;hx=26;hy=33;
                sx=(int(clock_*16.f)%5)*51;
            }else if(h.kind==Hazard::Bugsy){
                w=48;hh=32;hx=24;hy=32;
                sx=(int(clock_*12.f)%4)*48;sy=32;
            }else{
                sx=10;sy=80;
            }
            const auto i=int(h.kind);
            ui.image(tex_[i],{(h.x-hx-cx)*2.f,(h.y-hy-cy)*2.f,w*2.f,hh*2.f},
                {float(sx)/sheet_w_[i],float(sy)/sheet_h_[i],
                 float(w)/sheet_w_[i],float(hh)/sheet_h_[i]},white);
        }
        for(const auto& b:bullets_){
            if(b.x<cx-20.f||b.x>cx+980.f||b.y<cy-20.f||b.y>cy+560.f)continue;
            ui.image(tex_[2],{(b.x-8-cx)*2.f,(b.y-8-cy)*2.f,32,32},
                {160.f/sheet_w_[2],64.f/sheet_h_[2],
                 16.f/sheet_w_[2],16.f/sheet_h_[2]},white);
        }
    }
private:
    static std::string read(const std::string& file,size_t cap){
        FILE* f=std::fopen(file.c_str(),"rb");
        if(!f)return {};
        if(std::fseek(f,0,SEEK_END)){std::fclose(f);return {};}
        const long n=std::ftell(f);
        if(n<=0||static_cast<unsigned long>(n)>cap){std::fclose(f);return {};}
        std::rewind(f);
        std::string s(static_cast<size_t>(n),'\0');
        const bool ok=std::fread(s.data(),1,s.size(),f)==s.size();
        std::fclose(f);return ok?s:std::string();
    }
    static bool texture(hui::gfx::Renderer& r,const std::string& base,
                        const char* name,unsigned& id,int& w,int& h){
        const std::string prefix=base+name;
        const auto header=read(prefix+".txt",256);
        if(std::sscanf(header.c_str(),"%d %d",&w,&h)!=2 ||
           w<1||h<1||w>4096||h>4096)return false;
        const size_t size=size_t(w)*size_t(h)*4;
        const auto buffer=read(prefix+".rgba",size+1);
        if(buffer.size()!=size)return false;
        id=r.batch().create_texture(w,h,
             reinterpret_cast<const unsigned char*>(buffer.data()));
        return id!=0;
    }
    bool ready_=false;
    std::string error_;
    std::vector<Hazard> hazards_;
    std::vector<HazardBullet> bullets_;
    unsigned tex_[4]{};
    int sheet_w_[4]{},sheet_h_[4]{};
    int lives_=3,score_=0,defeated_=0,hits_=0;
    float invuln_=0,clock_=0;
};
}
