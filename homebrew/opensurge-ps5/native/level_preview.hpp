// SPDX-License-Identifier: GPL-3.0-or-later
// Open Surge PS5 v0.2.2 - indexed terrain collision + fixed-step motion preview.
// This is NOT the original Allegro/SurgeScript gameplay engine.
#pragma once
#include "gfx/renderer.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
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
struct BrickCell {std::uint64_t key=0;std::uint32_t index=0;};

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
    float velocity_x() const {return vx_;}
    size_t indexed_cells() const {return cells_.size();}

    bool load(Renderer& renderer,const std::string& game_root) {
        error_.clear(); placed_.clear(); cells_.clear(); atlas_=hero_=0;
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
        // Static cell index is built once, NEVER per frame.
        // The previous collision code linearly checked all 1323 bricks for
        // every foot/side pixel, causing severe stutter on the actual PS5.
        if(!build_grid()){
            error_="COLLISION CELL INDEX TOO LARGE";
            return false;
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
        // Level spawn is authored close to a CLOUD tile's masked surface,
        // which may be halfway through its 128px source rectangle.
        if(!has_foot_support(x_,y_)){
            const float spawn_y=y_;
            for(int k=1;k<=96;++k){
                if(has_foot_support(x_,spawn_y-float(k))){
                    y_=spawn_y-float(k);break;
                }
            }
            if(!has_foot_support(x_,y_)){
                for(int k=1;k<=64;++k){
                    if(has_foot_support(x_,spawn_y+float(k))){
                        y_=spawn_y+float(k);break;
                    }
                }
            }
        }
        ground_=has_foot_support(x_,y_);
    }
    // Fixed-step driver calls update at 120 Hz. No variable-per-frame slowdown.
    void update(float dt,int dir,bool jump){
        if(!ready())return;
        dt=std::clamp(dt,0.0f,1.f/60.f);
        dir=std::clamp(dir,-1,1);
        if(dir)facing_=dir;
        // Faster, predictable acceleration and friction (not original
        // Open Surge physics): aim for a responsive first playable prototype.
        const float speed=310.f;
        const float target=float(dir)*speed;
        const float accel=(ground_?(dir?2200.f:2600.f):1500.f)*dt;
        vx_+=std::clamp(target-vx_,-accel,accel);
        if(jump&&ground_){vy_=-505.f;ground_=false;}
        vy_=std::min(720.f,vy_+1200.f*dt);
        const float dx=vx_*dt,dy=vy_*dt;
        const int xsteps=std::max(1,int(std::ceil(std::abs(dx)/3.f)));
        const float step=dx/float(xsteps);
        for(int i=0;i<xsteps;++i){
            const float next=x_+step;
            if(!body_hits(next,y_)){x_=next;continue;}
            bool walked=false;
            if(ground_){
                for(int rise=1;rise<=7;++rise){
                    if(!body_hits(next,y_-float(rise))){
                        x_=next;y_-=float(rise);walked=true;break;
                    }
                }
            }
            if(!walked){vx_=0;break;}
        }
        const int ysteps=std::max(1,int(std::ceil(std::abs(dy)/3.f)));
        const float ystep=dy/float(ysteps);
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
        if(!ground_&&vy_>=0&&has_foot_support(x_,y_+1.5f)){
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
            // ps5-homebrew-ui Rect UV is (x,y,width,height), NOT
            // (u0,v0,u1,v1). Passing end-points caused texture bleeding.
            ui.image(atlas_,{left,top,float(def.w)*scale,float(def.h)*scale},
                     {u0,v0,u1-u0,v1-v0},white);
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
                     {facing_<0?u1:u0,v0,
                      facing_<0?u0-u1:u1-u0,v1-v0},white);
        }
    }
private:
    static std::uint64_t key_for(int cx,int cy) {
        return (std::uint64_t(std::uint32_t(cx))<<32) |
               std::uint64_t(std::uint32_t(cy));
    }
    static int cell_coord(float n) {
        return static_cast<int>(std::floor(n/128.f));
    }
    bool build_grid() {
        cells_.clear();
        // Original brick dimensions include 32x32 and 128x128 tiles. Index
        // every 128-world-unit cell overlapped by a tile, not only its origin.
        for(size_t i=0;i<placed_.size();++i) {
            const auto& b=placed_[i];
            const auto& d=defs_[b.id];
            if(!d.solid&&!d.cloud)continue;
            const int x0=cell_coord(float(b.x));
            const int x1=cell_coord(float(b.x+d.w-1));
            const int y0=cell_coord(float(b.y));
            const int y1=cell_coord(float(b.y+d.h-1));
            if((x1-x0+1)*(y1-y0+1)>256)return false;
            for(int x=x0;x<=x1;++x)
                for(int y=y0;y<=y1;++y)
                    cells_.push_back({key_for(x,y),std::uint32_t(i)});
            if(cells_.size()>200000)return false;
        }
        std::sort(cells_.begin(),cells_.end(),[](const BrickCell& a,const BrickCell& b){
            return a.key<b.key || (a.key==b.key && a.index<b.index);
        });
        return !cells_.empty();
    }
    template<typename Fn> void candidates(float px,float py,Fn fn) const {
        const auto key=key_for(cell_coord(px),cell_coord(py));
        const auto it=std::lower_bound(cells_.begin(),cells_.end(),key,
            [](const BrickCell& b,std::uint64_t wanted){return b.key<wanted;});
        for(auto cur=it;cur!=cells_.end()&&cur->key==key;++cur){
            fn(placed_[cur->index]);
        }
    }
    bool mask_pixel(const PlacedBrick& b,int sx,int sy) const {
        if(sx<0||sy<0||sx>=defs_[b.id].w||sy>=defs_[b.id].h)return false;
        const auto& d=defs_[b.id];
        const size_t offset=static_cast<size_t>(d.sy+sy)*atlasw_+(d.sx+sx);
        return offset<mask_.size()&&mask_[offset]!=0;
    }
    bool solid_at(float px,float py) const {
        bool hit=false;
        candidates(px,py,[&](const PlacedBrick& b){
            if(hit)return;
            const auto& d=defs_[b.id];
            if(!d.solid||px<float(b.x)||py<float(b.y)||
                px>=float(b.x+d.w)||py>=float(b.y+d.h))return;
            hit=mask_pixel(b,int(px-float(b.x)),int(py-float(b.y)));
        });
        return hit;
    }
    bool body_hits(float px,float feet) const {
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
            bool found_cloud=false;
            candidates(fx,feet,[&](const PlacedBrick& b){
                if(found_cloud)return;
                const auto& d=defs_[b.id];
                if(!d.cloud||fx<float(b.x)||fx>=float(b.x+d.w)||
                    feet<float(b.y)-1.5f||feet>=float(b.y+d.h))return;
                const int lx=int(fx-float(b.x));
                const int ly=std::max(0,int(feet-float(b.y)+1.f));
                found_cloud=mask_pixel(b,lx,ly);
            });
            if(found_cloud)return true;
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
    std::vector<BrickCell> cells_;
    std::vector<unsigned char> mask_;
    unsigned atlas_=0,hero_=0;
    int atlasw_=0,atlash_=0,herow_=0,heroh_=0;
    int visible_=0,facing_=1;
    float x_=0,y_=0,vx_=0,vy_=0,elapsed_=0,startx_=0,starty_=0;
    bool ground_=false;
    std::string error_;
};
} // namespace surgepreview
