// SPDX-License-Identifier: GPL-3.0-or-later
// Open Surge PS5 v0.2.0 - ORIGINAL brick+sprite level renderer and movement probe.
// This is NOT the original Allegro/SurgeScript gameplay engine.
#pragma once
#include "gfx/renderer.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

namespace surgepreview {
using namespace hui::gfx;
struct BrickDef {
    int sx=0,sy=0,w=0,h=0;
    bool solid=false,cloud=false,defined=false;
    float z=.5f;
};
struct PlacedBrick {int id=0,x=0,y=0;};

class LevelPreview {
public:
    static constexpr float scale=2.0f;
    bool ready() const {return atlas_!=0 && hero_!=0 && !placed_.empty();}
    const std::string& error() const {return error_;}
    size_t count() const {return placed_.size();}
    int visible() const {return visible_;}
    float player_x() const {return x_;}
    float player_y() const {return y_;}
    bool grounded() const {return ground_;}

    bool load(Renderer& renderer,const std::string& game_root) {
        error_.clear(); placed_.clear(); atlas_=hero_=0;
        if(game_root.size()<5 || game_root.substr(game_root.size()-5)!="game/") {
            error_="GAME ROOT INVALID";return false;
        }
        const std::string base=game_root.substr(0,game_root.size()-5)+"level-preview/";
        const auto hdr=read(base+"header.txt",4096);
        if(hdr.empty()){error_="PREVIEW DATA MISSING";return false;}
        int atlasw=0,atlash=0,herow=0,heroh=0;
        int spawnx=0,spawny=0,count=0;
        if(std::sscanf(hdr.c_str(),"atlas %d %d\nhero %d %d\nspawn %d %d\ninstances %d",
                       &atlasw,&atlash,&herow,&heroh,&spawnx,&spawny,&count)!=7){
            error_="PREVIEW HEADER INVALID";return false;
        }
        if(atlasw<1 || atlash<1 || herow<1 || heroh<1 ||
           atlasw>4096 || atlash>4096 || herow>4096 || heroh>4096 ||
           count<100 || count>40000) {
            error_="PREVIEW DIMENSIONS INVALID";return false;
        }
        atlasw_=atlasw;atlash_=atlash;herow_=herow;heroh_=heroh;
        const auto defs=read(base+"defs.txt",80*1024);
        std::istringstream defstream(defs);
        std::string line;
        int defined=0;
        while(std::getline(defstream,line)){
            std::istringstream row(line);
            int id=0,sx=0,sy=0,w=0,h=0;float z=.5f;std::string type;
            if(!(row>>id>>sx>>sy>>w>>h>>type>>z)) {error_="BRICK DEF PARSE";return false;}
            if(id<0||id>=512||defs_[id].defined||sx<0||sy<0||w<1||h<1||
               sx+w>atlasw||sy+h>atlash||!std::isfinite(z)){
                error_="BRICK DEF BOUNDS";return false;
            }
            if(type!="PASSABLE" && type!="SOLID" && type!="CLOUD"){
                error_="BRICK TYPE INVALID";return false;
            }
            defs_[id]={sx,sy,w,h,type=="SOLID",type=="CLOUD",true,z};
            ++defined;
        }
        if(defined<70){error_="BRICK DEFS INCOMPLETE";return false;}
        const auto positions=read(base+"bricks.txt",600000);
        std::istringstream instances(positions);
        while(std::getline(instances,line)){
            std::istringstream row(line);
            int id=0,bx=0,by=0;
            if(!(row>>id>>bx>>by) || id<0||id>=512||!defs_[id].defined ||
               bx<-1000000||bx>1000000||by<-1000000||by>1000000){
                error_="LEVEL BRICK PARSE";placed_.clear();return false;
            }
            placed_.push_back({id,bx,by});
        }
        if(static_cast<int>(placed_.size())!=count){
            error_="LEVEL BRICK COUNT";placed_.clear();return false;
        }
        const auto bg=read(base+"sunshine.rgba",static_cast<size_t>(atlasw)*atlash*4u+1);
        const auto player=read(base+"surge.rgba",static_cast<size_t>(herow)*heroh*4u+1);
        if(bg.size()!=static_cast<size_t>(atlasw)*atlash*4u ||
           player.size()!=static_cast<size_t>(herow)*heroh*4u) {
            error_="ORIGINAL TEXTURES INVALID";placed_.clear();return false;
        }
        atlas_=renderer.batch().create_texture(atlasw,atlash,
                             reinterpret_cast<const unsigned char*>(bg.data()));
        hero_=renderer.batch().create_texture(herow,heroh,
                            reinterpret_cast<const unsigned char*>(player.data()));
        if(!atlas_||!hero_){error_="PS5 TEXTURE UPLOAD FAILED";return false;}
        startx_=static_cast<float>(spawnx);
        starty_=static_cast<float>(spawny);
        reset();
        return true;
    }
    void reset(){
        x_=startx_; y_=starty_; vx_=vy_=0;ground_=false;
        elapsed_=0;
    }
    void update(float dt,int dir,bool jump){
        if(!ready())return;
        dt=std::clamp(dt,0.0f,0.04f);
        if(dir<-1)dir=-1; if(dir>1)dir=1;
        if(dir) facing_=dir;
        vx_=210.f*static_cast<float>(dir);
        if(jump && ground_){vy_=-465.f;ground_=false;}
        const float oldy=y_;
        float newx=x_+vx_*dt;
        float newy=y_+vy_*dt;
        vy_=std::min(650.f,vy_+1050.f*dt);
        // Initial flat-top collision probe; original alpha/mask, slopes and
        // moving/platform physics have NOT yet been ported.
        bool landing=false;
        if(vy_>=0){
            float best=newy;
            for(const auto& brick:placed_){
                const auto& def=defs_[brick.id];
                if(!def.solid && !def.cloud)continue;
                const float left=float(brick.x),top=float(brick.y);
                if(newx+15.f<left||newx-15.f>left+def.w)continue;
                if(oldy<=top+4.f && newy>=top && top<=best){
                    best=top;landing=true;
                }
            }
            if(landing){newy=best;vy_=0;}
        }
        x_=newx;y_=newy;ground_=landing;
        if(std::abs(y_-starty_)>3000.f)reset();
        elapsed_+=dt;
    }
    void draw(DrawList& ui) {
        const Color white=Color::rgb(0xffffff);
        ui.gradient_rect({0,0,1920,1080},0,Color::rgb(0x3bbbd9),Color::rgb(0xaddcce));
        const float camx=x_-480.f,camy=y_-270.f;
        visible_=0;
        for(const auto& brick:placed_){
            const auto& def=defs_[brick.id];
            const float worldx=float(brick.x),worldy=float(brick.y);
            if(worldx+def.w<camx||worldx>camx+960 ||
               worldy+def.h<camy||worldy>camy+540)continue;
            const float left=(worldx-camx)*scale,top=(worldy-camy)*scale;
            const float u0=float(def.sx)/atlasw_,v0=float(def.sy)/atlash_;
            const float u1=float(def.sx+def.w)/atlasw_,v1=float(def.sy+def.h)/atlash_;
            ui.image(atlas_,{left,top,float(def.w)*scale,float(def.h)*scale},
                     {u0,v0,u1,v1},white);
            if(++visible_>=1200)break;
        }
        int frame=0;
        if(!ground_){frame=19+(int(elapsed_*25.f)%8);}
        else if(vx_!=0){frame=1+(int(elapsed_*18.f)%8);}
        const int col=frame%8,row=frame/8;
        const int sx=col*64,sy=32+row*64;
        if(sx+64<=herow_ && sy+64<=heroh_){
            const float hx=(x_-32-camx)*scale,hy=(y_-44-camy)*scale;
            const float u0=float(sx)/herow_,v0=float(sy)/heroh_;
            const float u1=float(sx+64)/herow_,v1=float(sy+64)/heroh_;
            // Image UV horizontal inversion for leftward movement.
            ui.image(hero_,{hx,hy,128,128},
                     {facing_<0?u1:u0,v0,facing_<0?u0:u1,v1},white);
        }
    }
private:
    static std::string read(const std::string& path,size_t max){
        if(FILE* file=std::fopen(path.c_str(),"rb")){
            if(std::fseek(file,0,SEEK_END)){std::fclose(file);return {};}
            const long len=std::ftell(file);
            if(len<=0 || static_cast<unsigned long>(len)>max){
                std::fclose(file);return {};
            }
            std::rewind(file);
            std::string text(static_cast<size_t>(len),'\0');
            const bool ok=std::fread(text.data(),1,text.size(),file)==text.size();
            std::fclose(file);
            return ok?text:std::string();
        }
        return {};
    }
    BrickDef defs_[512]{};
    std::vector<PlacedBrick> placed_;
    unsigned atlas_=0,hero_=0;
    int atlasw_=0,atlash_=0,herow_=0,heroh_=0;
    int visible_=0,facing_=1;
    float x_=0,y_=0,vx_=0,vy_=0,elapsed_=0,startx_=0,starty_=0;
    bool ground_=false;
    std::string error_;
};
} // namespace surgepreview
