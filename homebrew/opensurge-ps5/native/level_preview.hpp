// SPDX-License-Identifier: GPL-3.0-or-later
// Open Surge PS5 v0.2.1 - transparent original sprites + mask-based collision probe.
// This is NOT the original Allegro/SurgeScript gameplay engine.
#pragma once
#include "gfx/renderer.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
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
    float velocity_y() const {return vy_;}

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
        int defined=0;
        size_t index=0;
        while(index<defs.size()){
            const auto end=defs.find('\n',index);
            const auto length=end==std::string::npos?defs.size()-index:end-index;
            if(length==0){index=end==std::string::npos?defs.size():end+1;continue;}
            const auto row=defs.substr(index,length);
            int id=0,sx=0,sy=0,w=0,h=0;float z=.5f;char type[16]{};
            if(std::sscanf(row.c_str(),"%d %d %d %d %d %15s %f",
                           &id,&sx,&sy,&w,&h,type,&z)!=7){
                error_="BRICK DEF PARSE";return false;
            }
            if(id<0||id>=512||defs_[id].defined||sx<0||sy<0||w<1||h<1||
               sx+w>atlasw||sy+h>atlash||!std::isfinite(z)){
                error_="BRICK DEF BOUNDS";return false;
            }
            const std::string kind(type);
            if(kind!="PASSABLE" && kind!="SOLID" && kind!="CLOUD"){
                error_="BRICK TYPE INVALID";return false;
            }
            defs_[id]={sx,sy,w,h,kind=="SOLID",kind=="CLOUD",true,z};
            ++defined;
            index=end==std::string::npos?defs.size():end+1;
        }
        if(defined<70){error_="BRICK DEFS INCOMPLETE";return false;}
        const auto positions=read(base+"bricks.txt",600000);
        index=0;
        while(index<positions.size()){
            const auto end=positions.find('\n',index);
            const auto length=end==std::string::npos?positions.size()-index:end-index;
            if(length==0){index=end==std::string::npos?positions.size():end+1;continue;}
            const auto row=positions.substr(index,length);
            int id=0,bx=0,by=0;
            if(std::sscanf(row.c_str(),"%d %d %d",&id,&bx,&by)!=3 ||
               id<0||id>=512||!defs_[id].defined ||
               bx<-1000000||bx>1000000||by<-1000000||by>1000000){
                error_="LEVEL BRICK PARSE";placed_.clear();return false;
            }
            placed_.push_back({id,bx,by});
            index=end==std::string::npos?positions.size():end+1;
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

        // The mask comes from Open Surge's separate, authentic brick collision
        // image; no arbitrary 128x128 box collider for passable decorations.
        const auto collision=read(base+"collision.bin",
            static_cast<size_t>(atlasw)*atlash+1);
        if(collision.size()!=static_cast<size_t>(atlasw)*atlash){
            error_="ORIGINAL COLLISION MASK MISSING";return false;
        }
        mask_.assign(collision.begin(),collision.end());
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
        x_=startx_; y_=starty_; vx_=vy_=0;ground_=false;facing_=1;
        elapsed_=0;
        // The spawn point is the character's source hot spot, not the bitmap
        // bottom. Support slightly embedded spawn coordinates without snapping
        // hundreds of pixels to an arbitrary platform.
        for(int k=0;k<32 && body_hits(x_,y_);++k)y_-=1.0f;
        for(int k=0;k<48 && !has_foot_support(x_,y_);++k)y_+=1.0f;
        ground_=has_foot_support(x_,y_);
    }
    void update(float dt,int dir,bool jump){
        if(!ready())return;
        dt=std::clamp(dt,0.0f,0.035f);
        dir=std::clamp(dir,-1,1);
        if(dir)facing_=dir;
        // Non-original experimental movement; slope masks now govern ground.
        const float target=dir*250.0f;
        const float change=(ground_?1700.f:1000.f)*dt;
        vx_+=std::clamp(target-vx_,-change,change);
        if(jump&&ground_){vy_=-490.0f;ground_=false;}
        vy_=std::min(700.0f,vy_+1150.0f*dt);
        const float dx=vx_*dt,dy=vy_*dt;
        const int xsteps=std::max(1,int(std::ceil(std::abs(dx)/2.f)));
        const float step=dx/xsteps;
        for(int i=0;i<xsteps;++i){
            const float next=x_+step;
            if(!body_hits(next,y_)){x_=next;continue;}
            bool walked=false;
            if(ground_){
                // Limited, pixel-mask-derived step-up for gentler slopes.
                for(int rise=1;rise<=8;++rise){
                    if(!body_hits(next,y_-rise)){
                        x_=next;y_-=rise;walked=true;break;
                    }
                }
            }
            if(!walked){vx_=0;break;}
        }
        const int ysteps=std::max(1,int(std::ceil(std::abs(dy)/2.f)));
        const float ystep=dy/ysteps;
        bool landed=false;
        for(int i=0;i<ysteps;++i){
            const float next=y_+ystep;
            if(ystep>=0){
                if(has_foot_support(x_,next)){vy_=0;landed=true;break;}
            }else if(head_hits(x_,next)){
                vy_=0;break;
            }
            y_=next;
        }
        ground_=landed;
        if(!ground_ && vy_>=0 && has_foot_support(x_,y_+1.5f)){
            ground_=true;vy_=0;
        }
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
            // A subtle marker stays behind the character until GPU blending
            // and the source-frame visibility are validated on PS5 hardware.
            ui.rounded_rect({hx+43,hy+94,42,24},12,
                            Color::rgb(0x003344,0.35f));
            ui.image(hero_,{hx,hy,128,128},
                     {facing_<0?u1:u0,v0,facing_<0?u0:u1,v1},white);
        }
    }
private:
    bool mask_pixel(const PlacedBrick& b,int sx,int sy) const {
        if(sx<0||sy<0||sx>=defs_[b.id].w||sy>=defs_[b.id].h)return false;
        const auto& d=defs_[b.id];
        const size_t offset=static_cast<size_t>(d.sy+sy)*atlasw_+(d.sx+sx);
        return offset<mask_.size()&&mask_[offset]!=0;
    }
    bool solid_at(float px,float py) const {
        for(const auto& b:placed_){
            const auto& d=defs_[b.id];
            if(!d.solid||px<float(b.x)||py<float(b.y)||
                px>=float(b.x+d.w)||py>=float(b.y+d.h))continue;
            if(mask_pixel(b,int(px-float(b.x)),int(py-float(b.y))))
                return true;
        }
        return false;
    }
    bool body_hits(float px,float feet) const{
        for(const float yy:{feet-38.f,feet-22.f,feet-8.f}){
            if(solid_at(px-12.f,yy)||solid_at(px+12.f,yy))return true;
        }
        return false;
    }
    bool head_hits(float px,float feet) const {
        return solid_at(px-11.f,feet-44.f)||solid_at(px+11.f,feet-44.f);
    }
    bool has_foot_support(float px,float feet) const {
        for(float fx:{px-10.f,px,px+10.f}){
            if(solid_at(fx,feet+1.f))return true;
            for(const auto& b:placed_){
                const auto& d=defs_[b.id];
                if(!d.cloud||fx<float(b.x)||fx>=float(b.x+d.w)||
                    feet<float(b.y)-1.5f||feet>float(b.y)+2.5f)continue;
                // Passable cloud platform is only solid on its upper edge.
                if(mask_pixel(b,int(fx-b.x),0))return true;
            }
        }
        return false;
    }
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
    std::vector<unsigned char> mask_;
    unsigned atlas_=0,hero_=0;
    int atlasw_=0,atlash_=0,herow_=0,heroh_=0;
    int visible_=0,facing_=1;
    float x_=0,y_=0,vx_=0,vy_=0,elapsed_=0,startx_=0,starty_=0;
    bool ground_=false;
    std::string error_;
};
} // namespace surgepreview
