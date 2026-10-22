// SPDX-License-Identifier: GPL-3.0-or-later
// Open Surge PS5 v0.5.1 - true rolling capsule sensors, slope momentum and uphill collision repair.
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
    unsigned collision_blocks() const {return blocked_;}
    unsigned auto_respawns() const {return respawns_;}
    float slope_grade() const {return slope_grade_;}
    unsigned uphill_steps() const {return uphill_steps_;}
    bool rolling() const {return rolling_;}
    bool charging() const {return charging_;}
    bool crouching() const {return ground_ && down_ && !rolling_ && !charging_ && std::abs(vx_)<60.f;}
    bool looking_up() const {return ground_ && up_ && !down_ && !rolling_ && std::abs(vx_)<60.f;}
    float dash_charge() const {return charge_;}
    unsigned dash_releases() const {return dash_releases_;}
    int sprite_frame() const {
        static constexpr int spin[12]={19,20,20,21,22,22,23,24,24,25,26,26};
        static constexpr int brake[4]={29,30,31,32};
        if(charging_)return 20+2*(int(elapsed_*40.f)%4);
        if(rolling_ || !ground_ && vy_<0.f)
            return spin[int(elapsed_*40.f)%12];
        if(!ground_ && vy_>=0.f)
            return 42+(int(elapsed_*20.f)%4);
        if(crouching())return 39;
        if(looking_up())return 36;
        if(braking_)return brake[int(elapsed_*16.f)%4];
        if(spring_anim_>0.f)return 40+(int(elapsed_*16.f)%2);
        if(std::abs(vx_)>245.f)return 9+(int(elapsed_*20.f)%8);
        if(std::abs(vx_)>10.f)return 1+(int(elapsed_*20.f)%8);
        return 0;
    }
#ifdef SURGEPREVIEW_HOST_TEST
    // Host-only way to reproduce the exact slope position from a PS5 user
    // screenshot (x=2500,y=10015); this is absent from the shipping FSELF.
    void test_pixel(float px,float py) const {
        std::printf("PIXEL world=%.1f,%.1f\n",px,py);
        candidates(px,py,[&](const PlacedBrick& b){
            const auto& d=defs_[b.id];
            if(px<float(b.x)||py<float(b.y)||px>=float(b.x+d.w)||
               py>=float(b.y+d.h))return;
            const bool opaque=mask_pixel(b,int(px-float(b.x)),int(py-float(b.y)));
            std::printf("   BRICK id=%d pos=(%d,%d) size=(%d,%d) z=%.2f solid=%d cloud=%d mask=%d\n",
                 b.id,b.x,b.y,d.w,d.h,d.z,d.solid?1:0,d.cloud?1:0,opaque?1:0);
        });
    }
    void test_probe(float dx) const {
        const float nx=x_+dx;
        float floor=y_;
        const bool support=find_floor(nx,y_,12,22,floor);
        float forward=0.f,far=0.f;
        bool front_floor=floor_height_at(nx+7.f,y_,110,32,forward);
        bool far_floor=floor_height_at(nx+12.f,y_,110,32,far);
        std::printf("PROBE dx=%+.1f x=%.1f feet=%.1f detected=%d floor=%.1f roll=%d hit=%d front_mid=%d front_top=%d front_floor=%d:%.1f far_floor=%d:%.1f\n",
            dx,nx,y_,support?1:0,floor,rolling_?1:0,
            body_hits(nx,support?floor:y_)?1:0,
            solid_at(nx+7.f,(support?floor:y_)-16.f)?1:0,
            solid_at(nx+7.f,(support?floor:y_)-26.f)?1:0,
            front_floor?1:0,forward,far_floor?1:0,far);
    }
    bool test_position(float px,float feet){
        x_=px;y_=feet;vx_=vy_=0;
        rolling_=charging_=false;charge_=0.f;
        float surface=feet;
        if(!find_floor(px,feet,24,24,surface))return false;
        y_=surface;ground_=true;last_ground_y_=y_;
        // The PS5 screenshot reproduces an initial body/terrain overlap:
        // that's precisely what the rolling sensor update should resolve.
        // Ground placement must not reject it before exercising the dash.
        return true;
    }
#endif
    // Experimental native spring velocity hook from original 'Spring
    // Standard/Stronger' SurgeScript values (600 and 960 world units/sec).
    void hit_recoil(float source_x) {
        if(!ready())return;
        vx_=(x_>=source_x?1.f:-1.f)*210.f;
        vy_=-280.f;
        ground_=false;
        charging_=rolling_=false;
        charge_=0.f;
        y_-=4.f;
    }
    void spring_bounce(float strength) {
        if(!ready()||strength<100.f||strength>1400.f)return;
        vy_=-strength;
        ground_=false;spring_anim_=0.32f;
        charging_=false;charge_=0.f;rolling_=false;
        y_-=3.f; // free the player's feet from the spring platform
    }

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
        blocked_=0;last_ground_y_=y_;
        elapsed_=0;down_=up_=rolling_=charging_=braking_=false;
        charge_=0.f;spring_anim_=0.f;dash_releases_=0;
        slope_grade_=0.f;uphill_steps_=0;
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
        last_ground_y_=y_;
    }
    // Stable 120Hz step. Map dimensions and original mask stay unchanged.
    // IMPORTANT: walking on a slope follows the surface before checking a
    // horizontal wall; the old code collided with the ground as though it
    // were a wall, setting VX=0 at x≈723/900 despite a 60 FPS display.
    void update(float dt,int dir,bool jump,bool down=false,bool up=false){
        if(!ready())return;
        dt=std::clamp(dt,0.f,1.f/60.f);
        dir=std::clamp(dir,-1,1);
        down_=down;up_=up;
        spring_anim_=std::max(0.f,spring_anim_-dt);
        // See upstream physicsactor.c (Open Surge 0.6.1.3):
        // CHARGING: tap fire1 repeatedly to add 0.25 intensity; release DOWN
        // to launch gsp = (12*60/3)*(2+intensity) = 480..720 world px/s.
        // Do NOT turn a DOWN+X charge into a jump.
        if(charging_){
            vx_=0.f;
            charge_=std::max(0.f,charge_*(1.f-1.88f*dt));
            if(down){
                if(jump)charge_=std::min(1.f,charge_+0.25f);
                elapsed_+=dt;
                return;
            }
            vx_=(facing_>=0?1.f:-1.f)*(480.f+240.f*charge_);
            charge_=0.f;charging_=false;rolling_=true;
            ++dash_releases_;
        }else if(ground_ && down && jump && std::abs(vx_)<60.f && !rolling_){
            charging_=true;charge_=0.f;vx_=0.f;
            elapsed_+=dt;
            return;
        }
        if(dir!=0 && !rolling_)facing_=dir;
        if(ground_ && down && std::abs(vx_)>=60.f && !rolling_)rolling_=true;
        braking_=ground_ && !rolling_ && dir!=0 && vx_*dir<0.f &&
                 std::abs(vx_)>220.f;
        if(rolling_){
            // Source physicsactor.c: rolling sensors are NARROWER and LOWER
            // than upright sensors (15x29 vs 21x38). Keeping upright side
            // probes here made a solid uphill slope look like a vertical wall
            // and set VX=0, despite a smooth 60 FPS in the user screenshot.
            //
            // Source rolling friction = 84.375 world px/s2. Source rolling
            // uphill gravity = 281.25 * sin(slope) px/s2; downhill 1125.
            // Our native mask traces actual surface height, so derive slope
            // from two samples on that mask, not arbitrary sprite artwork.
            slope_grade_=0.f;
            if(ground_){
                float left=0.f,right=0.f;
                if(floor_height_at(x_-9.f,y_,15,24,left) &&
                   floor_height_at(x_+9.f,y_,15,24,right)){
                    const float grade=std::clamp((left-right)/18.f,-3.f,3.f);
                    slope_grade_=grade;
                    const float sin_slope=grade/std::sqrt(1.f+grade*grade);
                    const bool uphill=vx_*sin_slope>0.f;
                    const float slope_force=uphill?281.25f:1125.f;
                    vx_-=sin_slope*slope_force*dt;
                }
            }
            const float friction=(dir!=0 && vx_*dir<0.f)?450.f:84.375f;
            const float delta=friction*dt;
            if(std::abs(vx_)<=delta)vx_=0.f;
            else vx_-=std::copysign(delta,vx_);
            if(ground_ && std::abs(vx_)<30.f)rolling_=false;
        }else{
            slope_grade_=0.f;
            const float target=(down && ground_)?0.f:dir*310.f;
            const float accel=(ground_?(dir?2200.f:2600.f):1500.f)*dt;
            vx_+=std::clamp(target-vx_,-accel,accel);
        }
        if(jump && ground_ && !down && !charging_){
            vy_=-505.f;ground_=false;
            rolling_=false;
        }
        // Keep ground contact stable across frame substeps and small changes
        // in mask height instead of falling and re-landing each single frame.
        if(ground_){
            float floor=0.f;
            if(find_floor(x_,y_,6,18,floor)){
                y_=floor;vy_=0.f;last_ground_y_=y_;
            }else ground_=false;
        }
        const float dx=vx_*dt;
        const int horizontal_steps=std::max(1,int(std::ceil(std::abs(dx)/2.f)));
        for(int i=0;i<horizontal_steps;++i){
            const float nx=x_+dx/float(horizontal_steps);
            float new_feet=y_;
            bool supported=false;
            if(ground_){
                // Probe center and both foot sensors on genuine mask surfaces.
                // Upward slopes <=12px and downward <=22px are walkable.
                supported=find_floor(nx,y_,12,22,new_feet);
            }
            if(supported){
                if(!body_hits(nx,new_feet)){
                    if(new_feet<y_-0.5f && rolling_)++uphill_steps_;
                    x_=nx;y_=new_feet;vy_=0.f;last_ground_y_=y_;
                    continue;
                }
                // Slope can become steep for the collision body before the
                // feet climb. Try a small upward correction, never phasing
                // through an actually tall wall.
                bool stepped=false;
                for(int rise=1;rise<=12;++rise){
                    const float ytry=new_feet-float(rise);
                    if(body_hits(nx,ytry))continue;
                    float floor=0.f;
                    if(find_floor(nx,ytry,1,3,floor) &&
                       std::abs(floor-ytry)<3.f){
                        if(floor<y_-0.5f && rolling_)++uphill_steps_;
                        x_=nx;y_=floor;last_ground_y_=y_;
                        stepped=true;break;
                    }
                }
                if(stepped)continue;
            }else if(!body_hits(nx,y_)){
                x_=nx;
                if(ground_)ground_=false; // genuine pit / platform edge
                continue;
            }
            // Real Sunshine demo uphill has two SOLID 128px bricks stacked
            // under the rendered slope (IDs 26 and 30 near world x=2500).
            // A high-energy roll can climb the connected masked upper face.
            // Spend kinetic energy on height, move in short continuous steps;
            // never bypass walls too tall for the available momentum.
            if(roll_uphill_step(nx,dx/float(horizontal_steps)))continue;
            vx_=0.f;
            if(dir!=0)++blocked_;
            break;
        }
        if(!ground_){
            const float prev_feet=y_;
            vy_=std::min(720.f,vy_+1200.f*dt);
            const float dy=vy_*dt;
            const int vertical_steps=std::max(1,int(std::ceil(std::abs(dy)/2.f)));
            const float step=dy/float(vertical_steps);
            for(int i=0;i<vertical_steps;++i){
                const float candidate=y_+step;
                if(step>=0.f){
                    float floor=0.f;
                    if(find_floor(x_,candidate,3,1,floor) &&
                       floor>=y_-1.f && floor<=candidate+1.f){
                        y_=floor;vy_=0.f;ground_=true;
                        last_ground_y_=y_;
                        break;
                    }
                }else if(head_hits(x_,candidate)){
                    vy_=0.f;break;
                }
                y_=candidate;
            }
            // If only holding RIGHT the original demo has real bottomless
            // gaps. Respawn from a long fall rather than scrolling the empty
            // sky for seconds without any level or character controls.
            if(y_>last_ground_y_+1050.f || y_>starty_+3000.f){
                ++respawns_;
                reset();
            }
        }
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
        const int frame=sprite_frame();
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
    bool roll_uphill_step(float next_x,float step_x) {
        if(!rolling_||charging_||std::abs(vx_)<205.f ||
           std::abs(y_-last_ground_y_)>36.f)return false;
        // The original roll-uphill force is 281.25, NOT world gravity 1200.
        // Converting height into speed using raw gravity drained energy
        // four times faster than the upstream rolling physics.
        constexpr float kRollHillResistance=281.25f;
        const float energy_height=(vx_*vx_)/(2.f*kRollHillResistance);
        // Allow at most 220 world-pixel rise (original demo steep hill), but
        // require real free headroom on its upper SOLID mask surface.
        const int max_rise=std::min(220,int(energy_height)+4);
        if(max_rise<16)return false;
        float upper=0.f;
        if(!find_floor(next_x,y_,max_rise,12,upper) ||
           upper>=y_-8.f || upper<y_-float(max_rise) ||
           body_hits(next_x,upper))return false;
        const float rise=std::min(y_-upper,std::min(4.f,std::abs(step_x)*2.f));
        if(rise<=0.f)return false;
        const float energy_after=std::max(0.f,vx_*vx_-2.f*kRollHillResistance*rise);
        if(energy_after<205.f*205.f)return false;
        x_=next_x;
        y_-=rise;
        vx_=std::copysign(std::sqrt(energy_after),vx_);
        vy_=0.f;
        ground_=true;
        last_ground_y_=y_;
        ++uphill_steps_;
        return true;
    }
    bool body_hits(float px,float feet) const {
        // Open Surge rolls with a 15px width, ~29px height sensor (original
        // physicsactor.c), NOT its normal standing 24x38 probe. This is vital
        // when racing up real masked inclines: an upright front corner will
        // penetrate the uphill terrain long before the feet reach its surface.
        if(rolling_ || charging_){
            for(const float yy:{feet-25.f,feet-16.f,feet-8.f}){
                if(solid_at(px-7.f,yy)||solid_at(px+7.f,yy))return true;
            }
        }else{
            for(const float yy:{feet-38.f,feet-22.f,feet-8.f}){
                if(solid_at(px-12.f,yy)||solid_at(px+12.f,yy))return true;
            }
        }
        return false;
    }
    bool head_hits(float px,float feet) const {
        return solid_at(px-11.f,feet-44.f)||solid_at(px+11.f,feet-44.f);
    }
    bool floor_pixel(float px,float py) const {
        if(solid_at(px,py))return true;
        bool cloudy=false;
        candidates(px,py,[&](const PlacedBrick& b){
            if(cloudy)return;
            const auto& d=defs_[b.id];
            if(!d.cloud||px<float(b.x)||px>=float(b.x+d.w)||
               py<float(b.y)||py>=float(b.y+d.h))return;
            cloudy=mask_pixel(b,int(px-float(b.x)),int(py-float(b.y)));
        });
        return cloudy;
    }
    // A surface is the first occupied pixel BELOW transparent air. Searching
    // at a foot height (rather than checking any solid pixel near the player)
    // avoids false "landed" transitions inside solid terrain.
    bool find_floor(float px,float feet,int rise,int drop,float& out) const {
        bool found=false;
        float surface=0.f;
        const int lo=int(std::floor(feet))-rise;
        const int hi=int(std::ceil(feet))+drop;
        for(float sensor:{px-9.f,px,px+9.f}){
            for(int row=lo;row<=hi;++row){
                const float at=float(row);
                if(!floor_pixel(sensor,at)||floor_pixel(sensor,at-1.f))continue;
                // Prefer the highest reachable top for uphill walking.
                if(!found || at<surface){
                    surface=at;found=true;
                }
            }
        }
        if(found)out=surface;
        return found;
    }
    bool floor_height_at(float px,float feet,int rise,int drop,float& result) const {
        // Same top-of-mask transition used for collision, single sensor to
        // measure the geometric slope without mixing nearby brick surfaces.
        const int lo=int(std::floor(feet))-rise,hi=int(std::ceil(feet))+drop;
        for(int row=lo;row<=hi;++row){
            if(floor_pixel(px,float(row)) && !floor_pixel(px,float(row)-1.f)){
                result=float(row);return true;
            }
        }
        return false;
    }
    bool has_foot_support(float px,float feet) const {
        float floor=0.f;
        return find_floor(px,feet,2,2,floor) && std::abs(floor-feet)<=2.f;
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
    bool down_=false,up_=false,rolling_=false,charging_=false,braking_=false;
    float charge_=0.f,spring_anim_=0.f;
    float slope_grade_=0.f;
    unsigned uphill_steps_=0;
    unsigned dash_releases_=0;
    float last_ground_y_=0.f;
    unsigned blocked_=0,respawns_=0;
    std::string error_;
};
} // namespace surgepreview
