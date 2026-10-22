// SPDX-License-Identifier: GPL-3.0-or-later
// Open Surge PS5 v0.6.2 - keep high-speed uphill steps mask-safe within one physics frame.
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
struct PlacedBrick {int id=0,x=0,y=0;unsigned char layer=0;bool hflip=false;};
struct ScriptRoute {
    enum Kind : unsigned char { Green,Yellow,TubeIn,TubeOut } kind=Green;
    float x=0,y=0;
};
struct BrickCell {std::uint64_t key=0;std::uint32_t index=0;};
// Tube In/Out are control locks in original tubes.ss, not teleporters.
// Follow the original brick collision corridor while input is locked.
struct TubeTrackPoint {float x=0.f,y=0.f,distance=0.f;};
// Native loop compatibility sensor. Geometry is discovered from the original
// paired Sunshine ring bricks; radius is read from their actual collision mask.
struct LoopRing {float x=0.f,y=0.f,radius=0.f;};

class LevelPreview {
public:
    static constexpr float scale=2.0f;
    bool ready() const {return atlas_!=0 && hero_!=0 && !placed_.empty();}
    const std::string& error() const {return error_;}
    size_t count() const {return placed_.size();}
    int active_layer() const {return active_layer_;}
    unsigned layer_switches() const {return layer_switches_;}
    size_t route_count() const {return routes_.size();}
    unsigned tube_entries() const {return tube_entries_;}
    unsigned tube_exits() const {return tube_exits_;}
    unsigned unsafe_tube_exits() const {return unsafe_tube_exits_;}
    unsigned loop_safety_recoveries() const {return loop_safety_recoveries_;}
    bool in_tube() const {return tube_active_;}
    unsigned tube_track_samples() const {return unsigned(tube_track_.size());}
    float tube_track_distance() const {return tube_track_length_;}
    float tube_track_y_span() const {return tube_track_y_max_-tube_track_y_min_;}
    bool loop_mode_ready() const {return !loops_.empty();}
    size_t loop_count() const {return loops_.size();}
    bool looping() const {return loop_active_;}
    unsigned loop_completions() const {return loop_completions_;}
    // Floor, right wall, ceiling, left wall. Angles advance clockwise
    // around an interior ring, as in original physicsactor.c MM_*.
    int movement_mode() const {
        if(!loop_active_)return 0;
        const float a=std::fmod(loop_angle_+6.28318530718f,6.28318530718f);
        if(a<.78539816f||a>=5.49778714f)return 0;
        if(a<2.35619449f)return 1;
        if(a<3.92699082f)return 2;
        return 3;
    }

    int visible() const {return visible_;}
    float player_x() const {return x_;}
    float player_y() const {return y_;}
    bool grounded() const {return ground_;}
    float velocity_y() const {return vy_;}
    float velocity_x() const {return vx_;}
    size_t indexed_cells() const {return cells_.size();}
    unsigned collision_blocks() const {return blocked_;}
    unsigned auto_respawns() const {return respawns_;}
    unsigned solid_recoveries() const {return solid_recoveries_;}
    float slope_grade() const {return slope_grade_;}
    unsigned uphill_steps() const {return uphill_steps_;}
    bool wall_climbing() const {return wall_climb_;}
    unsigned wall_climbs() const {return wall_climbs_;}
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
    // Original-mask host harness, not compiled into PS5 shipping binaries.
    void test_set_rolling_speed(float value){
        vx_=value;rolling_=true;facing_=value>=0.f?1:-1;
    }
    bool test_setup_loop(size_t index,float speed) {
        if(index>=loops_.size() || std::abs(speed)<350.f)return false;
        const auto& ring=loops_[index];
        float r=0.f;
        if(!loop_radius_at(ring,0.f,r))return false;
        if(!loop_center_clear(ring.x,ring.y+r))return false;
        test_put(ring.x,ring.y+r+16.f,speed>=0.f?1:-1,true);
        vx_=speed;
        return true;
    }
    bool test_inside() const{return body_hits(x_,y_);}
    float test_safe_x() const{return safe_ground_x_;}
    float test_safe_y() const{return safe_ground_y_;}
    bool test_safe_body() const{return body_hits(safe_ground_x_,safe_ground_y_);}
    bool test_safe_support() const{return has_foot_support(safe_ground_x_,safe_ground_y_);}
    unsigned test_embed_ticks() const{return embedded_frames_;}

    void test_enable_routes(bool enabled){routes_enabled_=enabled;}
    void test_put(float x,float y,int facing=1,bool rolling=false){
        x_=x;y_=y;vx_=rolling?(facing>=0?300.f:-300.f):0.f;
        facing_=facing>=0?1:-1;
        vy_=0.f;rolling_=rolling;charging_=false;ground_=true;
        last_ground_y_=y_;
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
    bool set_checkpoint(float px,float py) {
        if(!ready()||!std::isfinite(px)||!std::isfinite(py)||
           std::abs(px)>100000.f||std::abs(py)>100000.f)return false;
        float floor=py;
        if(!find_floor(px,py,36,54,floor))return false;
        if(body_hits(px,floor))return false;
        startx_=px;starty_=floor;
        return true;
    }
    // Experimental native spring velocity hook from original 'Spring
    // Standard/Stronger' SurgeScript values (600 and 960 world units/sec).
    void hit_recoil(float source_x) {
        if(!ready())return;
        vx_=(x_>=source_x?1.f:-1.f)*210.f;
        vy_=-280.f;
        ground_=false;loop_active_=false;
        charging_=rolling_=false;
        charge_=0.f;
        y_-=4.f;
    }
    void spring_bounce(float strength) {
        if(!ready()||strength<100.f||strength>1400.f)return;
        vy_=-strength;
        ground_=false;loop_active_=false;spring_anim_=0.32f;
        charging_=false;charge_=0.f;rolling_=false;
        y_-=3.f; // free the player's feet from the spring platform
    }

    bool load(Renderer& renderer,const std::string& game_root,const std::string& stage="") {
        error_.clear(); placed_.clear(); cells_.clear(); routes_.clear(); atlas_=hero_=0;
        for(auto& def:defs_)def=BrickDef{};
        if(game_root.size()<5 || game_root.substr(game_root.size()-5)!="game/") {
            error_="GAME ROOT INVALID";return false;
        }
        const std::string parent=game_root.substr(0,game_root.size()-5);
        const std::string visual=parent+"level-preview/";
        const std::string base=stage.empty()?visual:parent+"stages/"+stage+"/";
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
            int id=0,bx=0,by=0;char lname[16]{},flag[16]{};
            const int parsed=std::sscanf(row.c_str(),"%d %d %d %15s %15s",&id,&bx,&by,lname,flag);
            if((parsed!=4 && parsed!=5) || (parsed==5 && std::string(flag)!="hflip") ||
               id<0||id>=512||!defs_[id].defined ||
               bx<-1000000||bx>1000000||by<-1000000||by>1000000){
                error_="LEVEL BRICK/LAYER PARSE";placed_.clear();return false;
            }
            const std::string layer(lname);
            if(layer!="default" && layer!="green" && layer!="yellow"){
                error_="UNSUPPORTED BRICK LAYER";return false;
            }
            placed_.push_back({id,bx,by,
                static_cast<unsigned char>(layer=="green"?1:layer=="yellow"?2:0),
                parsed==5});
            index=end==std::string::npos?positions.size():end+1;
        }
        if(static_cast<int>(placed_.size())!=count){
            error_="LEVEL BRICK COUNT";placed_.clear();return false;
        }
        // Original SurgeScript level entities: layer collision selectors and
        // tube input locking/unlocking. Not present in the old native preview.
        const auto scripted=read(base+"routes.tsv",16*1024);
        if(scripted.empty() && stage.empty()){error_="ORIGINAL ROUTES MISSING";return false;}
        size_t rp=0;int tube_in=0,tube_out=0,layer_points=0;
        while(rp<scripted.size()){
            const auto end=scripted.find('\n',rp);
            const std::string line=scripted.substr(rp,end==std::string::npos?
                                   std::string::npos:end-rp);
            if(!line.empty()){
                char name[24]{};int rx=0,ry=0;
                if(std::sscanf(line.c_str(),"%23s %d %d",name,&rx,&ry)!=3 ||
                   std::abs(rx)>100000||std::abs(ry)>100000){
                    error_="ORIGINAL ROUTE PARSE";return false;
                }
                ScriptRoute route;
                const std::string kind(name);
                if(kind=="green")route.kind=ScriptRoute::Green,++layer_points;
                else if(kind=="yellow")route.kind=ScriptRoute::Yellow,++layer_points;
                else if(kind=="tube_in")route.kind=ScriptRoute::TubeIn,++tube_in;
                else if(kind=="tube_out")route.kind=ScriptRoute::TubeOut,++tube_out;
                else{error_="UNKNOWN LEVEL SCRIPT ROUTE";return false;}
                route.x=float(rx);route.y=float(ry);
                routes_.push_back(route);
            }
            rp=end==std::string::npos?scripted.size():end+1;
        }
        if(stage.empty() && (tube_in!=6||tube_out!=6||layer_points<30)){
            error_="INCOMPLETE ORIGINAL ROUTES";return false;
        }
        // Static cell index is built once, NEVER per frame.
        // The previous collision code linearly checked all 1323 bricks for
        // every foot/side pixel, causing severe stutter on the actual PS5.
        if(!build_grid()){
            error_="COLLISION CELL INDEX TOO LARGE";
            return false;
        }
        const auto bg=read(visual+"sunshine.rgba",static_cast<size_t>(atlasw)*atlash*4u+1);
        const auto player=read(visual+"surge.rgba",static_cast<size_t>(herow)*heroh*4u+1);
        if(bg.size()!=static_cast<size_t>(atlasw)*atlash*4u ||
           player.size()!=static_cast<size_t>(herow)*heroh*4u) {
            error_="ORIGINAL TEXTURES INVALID";placed_.clear();return false;
        }

        // The mask comes from Open Surge's separate, authentic brick collision
        // image; no arbitrary 128x128 box collider for passable decorations.
        const auto collision=read(visual+"collision.bin",
            static_cast<size_t>(atlasw)*atlash+1);
        if(collision.size()!=static_cast<size_t>(atlasw)*atlash){
            error_="ORIGINAL COLLISION MASK MISSING";return false;
        }
        mask_.assign(collision.begin(),collision.end());
        build_loop_rings();
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
        wall_climb_=false;wall_climbs_=0;wall_top_=0.f;wall_dir_=0;
        embedded_frames_=solid_recoveries_=0;
        active_layer_=0;layer_switches_=0;tube_entries_=tube_exits_=0;
        tube_active_=false;tube_timer_=tube_cooldown_=0.f;
        tube_track_.clear();tube_track_index_=0;tube_track_length_=0.f;
        tube_track_y_min_=tube_track_y_max_=y_;
        unsafe_tube_exits_=loop_safety_recoveries_=0;
        safe_ground_x_=x_;safe_ground_y_=y_;
        tube_guard_=0.f;
        loop_active_=false;loop_angle_=loop_speed_=0.f;loop_index_=0;loop_completions_=0;
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
        safe_ground_x_=x_;safe_ground_y_=y_;
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
        tube_cooldown_=std::max(0.f,tube_cooldown_-dt);
        tube_guard_=std::max(0.f,tube_guard_-dt);
        if(tube_active_){
            tick_tube(dt);
            elapsed_+=dt;
            return;
        }
        if(loop_active_){
            if(jump){
                // Jump away from whichever loop surface is beneath Surge.
                const float nx=std::sin(loop_angle_);
                const float ny=std::cos(loop_angle_);
                vx_=float(loop_direction_)*loop_speed_*ny-505.f*nx;
                vy_=-float(loop_direction_)*loop_speed_*nx-505.f*ny;
                x_-=nx*3.f;y_-=ny*3.f;
                loop_active_=false;ground_=false;
            }else tick_loop(dt);
            elapsed_+=dt;
            return;
        }
        // For the original loop/ramp corridor our partial native movement
        // lacks MM_FLOOR/WALL/CEILING gravity rotation. v0.5.3 jumped out of
        // a tube into unsupported space and then fell under the entire map.
        // Until four-mode source physics is linked, fail safely to the last
        // real ground contact instead of losing the player below solid tiles.
        if((tube_guard_>0.f || layer_switches_>0) &&
           x_>=3500.f && x_<=4860.f && !ground_ &&
           y_>safe_ground_y_+130.f){
            restore_safe_ground();
            ++loop_safety_recoveries_;
            elapsed_+=dt;
            return;
        }
        tick_routes();

        if(!wall_climb_) {
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
            // Do not enlarge the rolling capsule to the standing collision
            // shape while its upper corner is still embedded in terrain.
            if(ground_ && std::abs(vx_)<30.f &&
               !standing_body_hits(x_,y_))rolling_=false;
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
        } // Avoid consuming Spin Dash energy or jumping while wall-climbing.
        // Steep original Sunshine bricks have a solid vertical face below
        // their upper surface. Treat a high-speed roll as a WALL-CONTACT
        // traversal, not an invitation to step x INTO opaque terrain.
        // Advance upwards at real frame speed, with x fixed at the obstacle;
        // on reaching the lip, only resume horizontal motion if the next
        // rolling body position is actually free of collision.
        if(wall_climb_){
            // Never decrease Y through a ceiling or another SOLID mask.
            // v0.6.1 lifted the actor without checking each intermediate
            // position and could let a fast dash tunnel through an overhang.
            const float rate=std::clamp(std::abs(vx_)*1.25f,280.f,760.f);
            const float rise=std::min(rate*dt,std::max(0.f,y_-(wall_top_-3.f)));
            const float previous=y_;
            // Native preview still lacks the original four-direction
            // slope sensor frame. A rising Sunshine masked hillside can
            // touch the actor's upright box while rolling against it. Keep
            // X stationary throughout the crest contact; only the *exit*
            // must prove clearance, so no horizontal tunneling occurs.
            // The previous v0.6.2 prototype checked upright box pixels
            // mid-ascent and stalled immediately at screenshot x=2500.
            y_-=rise;
            if(y_<previous-0.25f)++uphill_steps_;
            vy_=0.f;
            ground_=has_foot_support(x_,y_);
            if(y_<=wall_top_+1.f){
                const float next=x_+float(wall_dir_)*1.25f;
                if(!body_hits(next,y_) && !head_hits(next,y_)){
                    wall_climb_=false;
                    x_=next;
                    float floor=y_;
                    if(find_floor(x_,y_,4,10,floor) &&
                       !body_hits(x_,floor)){
                        y_=floor;ground_=true;last_ground_y_=y_;
                    }else{
                        // No solid support past the crest; fall normally,
                        // from the safe upper side of the actual tile.
                        ground_=false;vy_=0.f;
                    }
                }else if(y_<=wall_top_-34.f){
                    // A genuine vertical wall: don't climb it forever.
                    wall_climb_=false;vx_=0.f;vy_=0.f;
                    ground_=has_foot_support(x_,y_);
                }else {
                    y_-=std::min(2.f,rate*dt);
                }
            }
            elapsed_+=dt;
            return;
        }
        // Keep ground contact stable across frame substeps and small changes
        // in mask height instead of falling and re-landing each single frame.
        if(ground_){
            float floor=0.f;
            if(find_floor(x_,y_,6,18,floor) && !body_hits(x_,floor)){
                y_=floor;vy_=0.f;last_ground_y_=y_;
            }else ground_=false;
        }
        // Full loop motion is not horizontal: the tangent rotates into the
        // right wall, ceiling and left wall. Do this before floor/wall sweeps.
        if(try_enter_loop()){
            tick_loop(dt);
            elapsed_+=dt;
            return;
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
                       std::abs(floor-ytry)<3.f && !body_hits(nx,floor)){
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
            if(roll_uphill_step(nx,dx/float(horizontal_steps)))break;
            // Activation is a contact state, not a completed movement.
            // Do not process more horizontal substeps through the solid lip.
            vx_=0.f;
            if(dir!=0)++blocked_;
            break;
        }
        if(wall_climb_){
            // The climbing state is processed on the NEXT 120 Hz tick.
            // Applying normal gravity on this tick reintroduces the
            // exact through-terrain artifact caught by the console video.
            elapsed_+=dt;
            return;
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
                       floor>=y_-1.f && floor<=candidate+1.f &&
                       !body_hits(x_,floor)){
                        y_=floor;vy_=0.f;ground_=true;
                        last_ground_y_=y_;
                        break;
                    }
                    if(body_hits(x_,candidate)){
                        vy_=0.f;break;
                    }
                }else if(head_hits(x_,candidate)||body_hits(x_,candidate)){
                    vy_=0.f;break;
                }
                y_=candidate;
            }
            // If only holding RIGHT the original demo has real bottomless
            // gaps. Respawn from a long fall rather than scrolling the empty
            // sky for seconds without any level or character controls.
            if(y_>last_ground_y_+1050.f || y_>starty_+3000.f){
                const unsigned times=respawns_+1;
                reset();
                respawns_=times;
            }
        }
        tick_routes();
        // A small masked seam can overlap the actor for a tick or two.
        // However, partial corner penetration can persist indefinitely
        // without the CENTER/torso entering solid (observed Sunshine 3
        // x=1164.3 y=9920 for 250 ticks). After eight consecutive ticks,
        // recover to the last proven collision-free ground contact.
        // Never recover mid-wall-climb or while travelling through a tube.
        if(!wall_climb_ && !tube_active_ &&
           (body_hits(x_,y_) || torso_deep_inside_solid())){
            if(++embedded_frames_>=8){
                restore_safe_ground();
                ++solid_recoveries_;
                embedded_frames_=0;
            }
        }else embedded_frames_=0;
        if(ground_ && !wall_climb_ && !tube_active_ &&
           !standing_body_hits(x_,y_) && !body_hits(x_,y_) &&
           has_foot_support(x_,y_)){
            safe_ground_x_=x_;safe_ground_y_=y_;
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
            // Render both layers just like the original tilemap; collision
            // layer is independently selected by layer-switch entities.
            const float worldx=float(brick.x),worldy=float(brick.y);
            if(worldx+def.w<camx||worldx>camx+960 ||
               worldy+def.h<camy||worldy>camy+540)continue;
            const float left=(worldx-camx)*scale,top=(worldy-camy)*scale;
            const float u0=float(def.sx)/atlasw_,v0=float(def.sy)/atlash_;
            const float u1=float(def.sx+def.w)/atlasw_,v1=float(def.sy+def.h)/atlash_;
            // ps5-homebrew-ui Rect UV is (x,y,width,height), NOT
            // (u0,v0,u1,v1). Passing end-points caused texture bleeding.
            ui.image(atlas_,{left,top,float(def.w)*scale,float(def.h)*scale},
                     {brick.hflip?u1:u0,v0,
                      brick.hflip?u0-u1:u1-u0,v1-v0},white);
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
    bool brick_in_layer(const PlacedBrick& b) const {
        // Exactly upstream obstaclemap.c ignore_obstacle:
        // OL_DEFAULT player collides with all; green/yellow players collide
        // with default plus bricks belonging to the corresponding layer.
        return active_layer_==0||b.layer==0||b.layer==active_layer_;
    }
    // A tube may be animated through SOLID scenery only when the receiving
    // outlet has a physically valid player landing. The original SurgeScript
    // does special 4-mode tube/loop sensor handling. The preview doesn't.
    bool tube_exit_usable(float ex,float feet,float& floor) const {
        if(!find_floor(ex,feet,28,24,floor) ||
           std::abs(floor-feet)>24.f ||
           body_hits(ex,floor))return false;
        const float advance=ex+float(facing_)*12.f;
        float next=floor;
        if(!find_floor(advance,floor,12,24,next) ||
           next<floor-12.f || next>floor+24.f ||
           body_hits(advance,next))return false;
        return true;
    }
    void restore_safe_ground(){
        tube_active_=wall_climb_=loop_active_=false;
        tube_track_.clear();tube_track_index_=0;tube_track_length_=0.f;
        vx_=vy_=0.f;rolling_=charging_=false;
        // Rolling probes are narrower than standing probes; a saved rolling
        // location can be solid for the standing player. Never revive at a
        // location which only has foot support but intersects the terrain.
        const float saved_x=safe_ground_x_,saved_y=safe_ground_y_;
        auto try_ground=[&](float px,float feet){
            if(!std::isfinite(px)||!std::isfinite(feet))return false;
            float floor=feet;
            if(!find_floor(px,feet,16,24,floor) ||
               std::abs(floor-feet)>24.f ||
               body_hits(px,floor) || !has_foot_support(px,floor))
                return false;
            x_=px;y_=floor;ground_=true;
            return true;
        };
        bool restored=try_ground(saved_x,saved_y);
        if(!restored){
            // Emergency fallback, bounded to 96px around the last proven
            // ground contact. Never used for ordinary player movement.
            for(int dist=8;dist<=96&&!restored;dist+=8)
                restored=try_ground(saved_x-float(dist),saved_y) ||
                         try_ground(saved_x+float(dist),saved_y);
        }
        if(!restored){
            reset();
            ground_=has_foot_support(x_,y_)&&!body_hits(x_,y_);
            if(!ground_){
                for(int n=1;n<=80&&!ground_;++n){
                    if(try_ground(startx_,starty_-float(n)))break;
                    if(try_ground(startx_,starty_+float(n)))break;
                }
            }
        }
        last_ground_y_=y_;
        if(ground_&&!body_hits(x_,y_)){
            safe_ground_x_=x_;safe_ground_y_=y_;
        }
        embedded_frames_=0;
        tube_cooldown_=2.f;tube_guard_=0.f;
    }
    void tick_routes(){
        if(tube_active_||!routes_enabled_)return;
        for(const auto& route:routes_){
            if(std::abs(x_-route.x)>18.f||
               std::abs((y_-24.f)-route.y)>28.f)continue;
            if(route.kind==ScriptRoute::Green || route.kind==ScriptRoute::Yellow){
                const int next=route.kind==ScriptRoute::Green?1:2;
                if(active_layer_!=next){active_layer_=next;++layer_switches_;}
                continue;
            }
            if(route.kind!=ScriptRoute::TubeIn||tube_cooldown_>0.f)continue;
            const ScriptRoute* selected=nullptr;
            float best=1e7f;
            for(const auto& candidate:routes_){
                if(candidate.kind!=ScriptRoute::TubeOut)continue;
                const float dx=candidate.x-route.x;
                const float dy=candidate.y-route.y;
                const float forward=dx*float(facing_);
                if(forward<80.f||forward>1300.f||std::abs(dy)>45.f)continue;
                const float cost=forward+std::abs(dy)*4.f;
                if(cost<best){best=cost;selected=&candidate;}
            }
            if(selected && (rolling_||std::abs(vx_)>=100.f)){
                float support=0.f;
                const float exit_feet=selected->y+24.f;
                if(!tube_exit_usable(selected->x,exit_feet,support)){
                    ++unsafe_tube_exits_;
                    restore_safe_ground();
                    // Do not start a teleport that would deposit the player
                    // in midair or inside the original tile collision.
                    break;
                }
                if(ground_ && !body_hits(x_,y_)){
                    safe_ground_x_=x_;safe_ground_y_=y_;
                }
                // v0.6.3 linearly interpolated Tube In -> Tube Out. The S
                // tunnel then traversed with Y frozen at ~10064, visibly
                // crossing solid waves. Build an actual mask-following
                // trajectory before locking input; reject missing tracks.
                if(!build_tube_track(selected->x,support)){
                    ++unsafe_tube_exits_;
                    tube_cooldown_=0.3f;
                    break;
                }
                tube_active_=true;tube_from_x_=x_;tube_from_y_=y_;
                tube_to_x_=selected->x;tube_to_y_=support;
                tube_timer_=0.f;
                tube_duration_=std::max(0.05f,tube_track_length_/600.f);
                ++tube_entries_;
                rolling_=true;charging_=false;ground_=false;
                vy_=0.f;
            }
            break;
        }
    }

    bool nearest_tube_floor(float px,float prev_feet,float& out) const {
        // Follow an actual opaque-mask floor with transparent space above it,
        // rather than invent a sine curve from the artwork. Bounded local
        // scan prevents jumping to an unrelated level/platform at a seam.
        float best=1000.f;
        for(int d=-90;d<=90;++d){
            const float row=std::floor(prev_feet)+float(d);
            if(!floor_pixel(px,row) || floor_pixel(px,row-1.f))continue;
            if(solid_at(px,row-8.f) || solid_at(px,row-16.f))continue;
            const float delta=std::abs(row-prev_feet);
            if(delta<best){best=delta;out=row;}
        }
        // The original S curve moves 27-30px between some 4px X samples.
        // Keep a 40px bounded local contour tolerance, not a remote floor hop.
        return best<=40.f;
    }
    bool build_tube_track(float target_x,float target_y) {
        tube_track_.clear();tube_track_length_=0.f;tube_track_index_=0;
        tube_track_y_min_=tube_track_y_max_=y_;
        const float delta=target_x-x_;
        if(std::abs(delta)<30.f || std::abs(delta)>1300.f)return false;
        const float sign=delta>0.f?1.f:-1.f;
        tube_track_.push_back({x_,y_,0.f});
        // Original tubes.ss only locks input while physics moves the hero:
        // stepping along the source mask approximates that guided rolling.
        const float distance=std::abs(delta);
        for(float step=4.f;step<distance;step+=4.f){
            const auto& prev=tube_track_.back();
            const float px=x_+sign*step;
            float foot=prev.y;
            if(!nearest_tube_floor(px,prev.y,foot) ||
               std::abs(foot-prev.y)>40.f){
#ifdef SURGEPREVIEW_HOST_TEST
                std::printf("TUBE_TRACK_GAP x=%.1f prev_x=%.1f prev_y=%.1f sampled_y=%.1f\n",
                    px,prev.x,prev.y,foot);
#endif
                tube_track_.clear();return false;
            }
            tube_track_length_+=std::hypot(px-prev.x,foot-prev.y);
            tube_track_.push_back({px,foot,tube_track_length_});
            tube_track_y_min_=std::min(tube_track_y_min_,foot);
            tube_track_y_max_=std::max(tube_track_y_max_,foot);
        }
        const auto& prev=tube_track_.back();
        if(std::abs(prev.y-target_y)>32.f){
#ifdef SURGEPREVIEW_HOST_TEST
            std::printf("TUBE_TRACK_END_MISMATCH x=%.1f prev_y=%.1f target_y=%.1f\n",
                target_x,prev.y,target_y);
#endif
            tube_track_.clear();return false;
        }
        tube_track_length_+=std::hypot(target_x-prev.x,target_y-prev.y);
        tube_track_.push_back({target_x,target_y,tube_track_length_});
        tube_track_y_min_=std::min(tube_track_y_min_,target_y);
        tube_track_y_max_=std::max(tube_track_y_max_,target_y);
        return tube_track_.size()>=5 && tube_track_length_>30.f;
    }
    void tick_tube(float dt){
        if(!tube_active_)return;
        if(tube_track_.size()<2 || tube_track_length_<=0.f){
            ++unsafe_tube_exits_;
            restore_safe_ground();
            return;
        }
        tube_timer_=std::min(tube_duration_,tube_timer_+dt);
        const float travel=std::min(tube_track_length_,tube_timer_*600.f);
        while(tube_track_index_+2<tube_track_.size() &&
              tube_track_[tube_track_index_+1].distance<travel)
            ++tube_track_index_;
        const auto& a=tube_track_[tube_track_index_];
        const auto& b=tube_track_[tube_track_index_+1];
        const float segment=std::max(0.001f,b.distance-a.distance);
        const float t=std::clamp((travel-a.distance)/segment,0.f,1.f);
        x_=a.x+(b.x-a.x)*t;
        y_=a.y+(b.y-a.y)*t;
        vx_=0.f;vy_=0.f;
        if(tube_timer_>=tube_duration_){
            tube_active_=false;
            tube_track_.clear();tube_track_index_=0;
            float floor=y_;
            if(!tube_exit_usable(x_,y_,floor)){
                ++unsafe_tube_exits_;
                restore_safe_ground();
                return;
            }
            y_=floor;
            ground_=true;
            last_ground_y_=y_;
            safe_ground_x_=x_;safe_ground_y_=y_;
            ++tube_exits_;
            tube_cooldown_=1.2f;
            tube_guard_=1.5f;
            vx_=(facing_>=0?1.f:-1.f)*600.f;
            rolling_=true;
        }
    }

    static constexpr float kPi=3.14159265358979323846f;
    // Distance from the ring's empty center to an actual SOLID mask pixel.
    // Prevents hard-coded circular teleports through solid terrain.
    bool loop_radius_at(const LoopRing& ring,float angle,float& radius) const {
        const float nx=std::sin(angle),ny=std::cos(angle);
        if(solid_at(ring.x+nx*8.f,ring.y+ny*8.f))return false;
        for(int dist=20;dist<=128;++dist){
            if(solid_at(ring.x+nx*float(dist),ring.y+ny*float(dist))){
                radius=float(dist)-16.f;
                return radius>=12.f && radius<=112.f;
            }
        }
        return false;
    }
    bool loop_center_clear(float px,float py) const {
        // Do not use upright wall sensors in rotated contact states.
        return !solid_at(px,py) && !solid_at(px-5.f,py) &&
               !solid_at(px+5.f,py) && !solid_at(px,py-5.f) &&
               !solid_at(px,py+5.f);
    }
    void build_loop_rings(){
        loops_.clear();
        // Original demo's Loop In/Out art is built from lower Bricks 37/38
        // and upper Bricks 41/42. Derive positions instead of baking x=5888.
        for(const auto& left:placed_){
            if(left.id!=37)continue;
            bool right=false,upper_left=false,upper_right=false;
            for(const auto& candidate:placed_){
                if(candidate.id==38 && candidate.x==left.x+128 &&
                   candidate.y==left.y)right=true;
                if(candidate.id==41 && candidate.x==left.x &&
                   candidate.y==left.y-160)upper_left=true;
                if(candidate.id==42 && candidate.x==left.x+128 &&
                   candidate.y==left.y-160)upper_right=true;
            }
            if(!right||!upper_left||!upper_right)continue;
            LoopRing ring{float(left.x+128),float(left.y-16),0.f};
            if(solid_at(ring.x,ring.y))continue;
            float total=0.f;
            bool valid=true;
            for(int i=0;i<8;++i){
                float radius=0.f;
                if(!loop_radius_at(ring,float(i)*(kPi/4.f),radius)){
                    valid=false;break;
                }
                total+=radius;
            }
            if(valid){
                ring.radius=total/8.f;
                loops_.push_back(ring);
            }
        }
    }
    bool try_enter_loop(){
        if(loop_active_||!rolling_||charging_||std::abs(vx_)<350.f ||
           tube_active_||wall_climb_)return false;
        for(size_t i=0;i<loops_.size();++i){
            const auto& ring=loops_[i];
            if(std::abs(x_-ring.x)>16.f)continue;
            float r=0.f;
            if(!loop_radius_at(ring,0.f,r))continue;
            const float foot=ring.y+r+16.f;
            if(std::abs(y_-foot)>45.f)continue;
            const float centre_y=ring.y+r;
            if(!loop_center_clear(ring.x,centre_y))continue;
            loop_active_=true;loop_index_=i;loop_angle_=0.f;
            loop_direction_=vx_>=0.f?1:-1;
            loop_speed_=std::abs(vx_);
            x_=ring.x;y_=centre_y+16.f;vy_=0.f;
            ground_=true;wall_climb_=false;
            return true;
        }
        return false;
    }
    void tick_loop(float dt){
        if(!loop_active_||loop_index_>=loops_.size())return;
        const LoopRing& ring=loops_[loop_index_];
        // Project normal gravity onto tangent, matching the uphill energy
        // penalty and downhill acceleration rather than boosting through.
        const float incline=std::sin(loop_angle_)*float(loop_direction_);
        const float gravity_factor=incline>=0.f?281.25f:1125.f;
        loop_speed_=std::clamp(loop_speed_-gravity_factor*incline*dt,
                               0.f,960.f);
        if(loop_speed_<210.f){
#ifdef SURGEPREVIEW_HOST_TEST
            std::printf("LOOP_DETACH_SPEED angle=%.3f speed=%.1f layer=%d x=%.1f y=%.1f\n",
                loop_angle_,loop_speed_,active_layer_,x_,y_);
#endif
            // Insufficient momentum: detach in the current tangent direction.
            vx_=float(loop_direction_)*loop_speed_*std::cos(loop_angle_);
            vy_=-float(loop_direction_)*loop_speed_*std::sin(loop_angle_);
            loop_active_=false;ground_=false;
            return;
        }
        float remaining=loop_speed_*dt;
        while(remaining>0.0001f){
            const float ds=std::min(remaining,3.f);
            remaining-=ds;
            float radius=0.f;
            if(!loop_radius_at(ring,loop_angle_,radius)){
#ifdef SURGEPREVIEW_HOST_TEST
                std::printf("LOOP_DETACH_CONTACT angle=%.3f layer=%d x=%.1f y=%.1f\n",
                   loop_angle_,active_layer_,x_,y_);
#endif
                loop_active_=false;ground_=false;
                vx_=float(loop_direction_)*loop_speed_*std::cos(loop_angle_);
                vy_=-float(loop_direction_)*loop_speed_*std::sin(loop_angle_);
                return;
            }
            const float advance=float(loop_direction_)*ds/std::max(16.f,radius);
            const float next_angle=loop_direction_>0?
                std::min(2.f*kPi,loop_angle_+advance):
                std::max(-2.f*kPi,loop_angle_+advance);
            float next_radius=0.f;
            if(!loop_radius_at(ring,next_angle,next_radius)){
#ifdef SURGEPREVIEW_HOST_TEST
                std::printf("LOOP_DETACH_MASK angle=%.3f layer=%d x=%.1f y=%.1f\n",
                   next_angle,active_layer_,x_,y_);
#endif
                loop_active_=false;ground_=false;
                vx_=float(loop_direction_)*loop_speed_*std::cos(loop_angle_);
                vy_=-float(loop_direction_)*loop_speed_*std::sin(loop_angle_);
                return;
            }
            const float nx=ring.x+std::sin(next_angle)*next_radius;
            const float ny=ring.y+std::cos(next_angle)*next_radius;
            if(!loop_center_clear(nx,ny)){
#ifdef SURGEPREVIEW_HOST_TEST
                std::printf("LOOP_DETACH_BODY angle=%.3f layer=%d x=%.1f y=%.1f\n",
                   next_angle,active_layer_,nx,ny);
#endif
                loop_active_=false;ground_=false;
                vx_=loop_speed_*std::cos(loop_angle_);
                vy_=-loop_speed_*std::sin(loop_angle_);
                return;
            }
            loop_angle_=next_angle;
            x_=nx;y_=ny+16.f;
            if(std::abs(loop_angle_)>=2.f*kPi-0.0001f){
                loop_active_=false;++loop_completions_;
                loop_angle_=0.f;vx_=float(loop_direction_)*loop_speed_;vy_=0.f;
                ground_=has_foot_support(x_,y_);
                if(ground_ && !body_hits(x_,y_)){
                    safe_ground_x_=x_;safe_ground_y_=y_;
                    last_ground_y_=y_;
                }
                return;
            }
        }
        // Preserve the rolling speed independently of world X velocity.
        // Sprite/HUD velocity points along the ring tangent.
        vx_=float(loop_direction_)*loop_speed_*std::cos(loop_angle_);
        vy_=-float(loop_direction_)*loop_speed_*std::sin(loop_angle_);
        ground_=true;
    }
    bool mask_pixel(const PlacedBrick& b,int sx,int sy) const {
        if(sx<0||sy<0||sx>=defs_[b.id].w||sy>=defs_[b.id].h)return false;
        const auto& d=defs_[b.id];
        const size_t offset=static_cast<size_t>(d.sy+sy)*atlasw_+
                             (d.sx+(b.hflip?d.w-1-sx:sx));
        return offset<mask_.size()&&mask_[offset]!=0;
    }
    bool solid_at(float px,float py) const {
        bool hit=false;
        candidates(px,py,[&](const PlacedBrick& b){
            if(hit)return;
            if(!brick_in_layer(b))return;
            const auto& d=defs_[b.id];
            if(!d.solid||px<float(b.x)||py<float(b.y)||
                px>=float(b.x+d.w)||py>=float(b.y+d.h))return;
            hit=mask_pixel(b,int(px-float(b.x)),int(py-float(b.y)));
        });
        return hit;
    }
    bool roll_uphill_step(float next_x,float /*step_x*/) {
        if(!rolling_||charging_||std::abs(vx_)<305.f)return false;
        // The authored Sunshine ramp can briefly lose a foot sensor at its
        // tile seam even on a charged roll. Use the actual last foot contact
        // within a tightly bounded recovery window, not an unrestricted
        // mid-air wall climb (which could catch distant vertical scenery).
        if(!ground_){
            if(std::abs(y_-last_ground_y_)>40.f)return false;
            float old_floor=0.f;
            if(!find_floor(x_,y_,24,36,old_floor) ||
               std::abs(old_floor-y_)>36.f)return false;
        }
        const int dir=(vx_>=0.f)?1:-1;
        // Only cross a SOLID obstacle when it has an actual upper mask
        // surface above the current feet and space for the rolling body.
        // The previous v0.5.1 shortcut moved x while y rose only 2-4px,
        // which tunneled through 128px solid bricks and triggered freefall.
        // That shortcut is intentionally removed here.
        float top=0.f;
        if(!find_floor(next_x,y_,215,5,top) ||
           top>y_-22.f || top<y_-200.f ||
           body_hits(next_x,top-2.f))return false;
        // The upper part must be a real, contiguous solid lip, not a random
        // sprite or arbitrary air point. A small window behind the obstacle
        // must still be supported on the player's current side.
        if(ground_&&!has_foot_support(x_,y_))return false;
        wall_climb_=true;
        wall_top_=top-1.f;
        wall_dir_=dir;
        ++wall_climbs_;
        return true; // we did NOT move x or punch through the solid mask
    }
    bool standing_body_hits(float px,float feet) const {
        for(const float yy:{feet-38.f,feet-22.f,feet-8.f}){
            if(solid_at(px-12.f,yy)||solid_at(px+12.f,yy))return true;
        }
        return false;
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
        }else return standing_body_hits(px,feet);
        return false;
    }
    bool torso_deep_inside_solid() const {
        return solid_at(x_,y_-18.f) && solid_at(x_-3.f,y_-12.f) &&
               solid_at(x_+3.f,y_-12.f);
    }
    bool head_hits(float px,float feet) const {
        return solid_at(px-11.f,feet-44.f)||solid_at(px+11.f,feet-44.f);
    }
    bool floor_pixel(float px,float py) const {
        if(solid_at(px,py))return true;
        bool cloudy=false;
        candidates(px,py,[&](const PlacedBrick& b){
            if(cloudy)return;
            if(!brick_in_layer(b))return;
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
    std::vector<ScriptRoute> routes_;
    bool routes_enabled_=true;
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
    int active_layer_=0;
    unsigned layer_switches_=0,tube_entries_=0,tube_exits_=0;
    bool tube_active_=false;
    float tube_from_x_=0.f,tube_from_y_=0.f,tube_to_x_=0.f,tube_to_y_=0.f;
    float tube_timer_=0.f,tube_duration_=0.f,tube_cooldown_=0.f;
    std::vector<TubeTrackPoint> tube_track_;
    size_t tube_track_index_=0;
    float tube_track_length_=0.f,tube_track_y_min_=0.f,tube_track_y_max_=0.f;
    float tube_guard_=0.f,safe_ground_x_=0.f,safe_ground_y_=0.f;
    unsigned unsafe_tube_exits_=0,loop_safety_recoveries_=0;
    bool wall_climb_=false;
    float wall_top_=0.f;
    int wall_dir_=0;
    unsigned wall_climbs_=0;
    unsigned dash_releases_=0;
    float last_ground_y_=0.f;
    std::vector<LoopRing> loops_;
    bool loop_active_=false;
    size_t loop_index_=0;
    float loop_angle_=0.f,loop_speed_=0.f;
    int loop_direction_=1;
    unsigned loop_completions_=0;
    unsigned blocked_=0,respawns_=0;
    unsigned embedded_frames_=0,solid_recoveries_=0;
    std::string error_;
};
} // namespace surgepreview
