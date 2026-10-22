// SPDX-License-Identifier: GPL-3.0-or-later
// Open Surge native PS5: geometry-anchored scripted test rails for Sunshine.
// NOT a replacement for upstream MM_FLOOR/MM_WALL/MM_CEILING engine.
// These rails only engage at authored demo-1 tile boundaries at rolling speed.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace surgepreview {

class StageCurves {
public:
    enum class Mode: unsigned char { None, Serpentine, LoopApproach, LoopOrbit, LoopExit };
    struct Position {
        float x=0.f,y=0.f,vx=0.f,vy=0.f;
        bool completed=false;
    };
    void configure(bool snake_tiles,float snake_x,float snake_y,
                   bool ring_tiles,float ring_x,float ring_y) {
        snake_available_=snake_tiles;
        ring_available_=ring_tiles;
        snake_x_=snake_x;
        snake_y_=snake_y;
        ring_x_=ring_x;
        ring_y_=ring_y;
        reset();
    }
    void reset(){
        mode_=Mode::None;entries_=completions_=0;orbit_cycles_=0;
        speed_=0.f;pos_x_=pos_y_=angle_=0.f;
        min_y_=1.e8f;max_y_=-1.e8f;elapsed_=0.f;
    }
    void abort(){mode_=Mode::None;}
    bool ready() const{return snake_available_ && ring_available_;}
    bool active() const{return mode_!=Mode::None;}
    Mode mode() const{return mode_;}
    unsigned entries() const{return entries_;}
    unsigned completions() const{return completions_;}
    unsigned orbit_cycles() const{return orbit_cycles_;}
    float min_y() const{return min_y_;}
    float max_y() const{return max_y_;}
    float snake_start() const{return snake_x_;}
    float ring_center() const{return ring_x_;}
    static constexpr float kSnakeAmplitude=46.f;
    static constexpr float kSnakePeriod=256.f;
    static constexpr float kRingRadius=96.f;
    static constexpr float kPi=3.14159265358979323846f;

    float snake_path_y(float x) const {
        // Geometry-anchored against alternating ORIGINAL Sunshine brick
        // pairs 50/51 and 53/52. One full wave occupies two 128px tiles.
        return snake_y_-kSnakeAmplitude*std::cos(
            2.f*kPi*(x-snake_x_)/kSnakePeriod);
    }

    bool enter(float x,float y,float vx,bool rolling) {
        if(active()||!rolling||std::abs(vx)<300.f||vx<0.f)return false;
        if(snake_available_ && x>=snake_x_-28.f &&
           x<=snake_x_+50.f &&
           std::abs(y-snake_path_y(x))<92.f){
            mode_=Mode::Serpentine;speed_=std::clamp(std::abs(vx),340.f,725.f);
            pos_x_=x;pos_y_=y;elapsed_=0.f;++entries_;
            return true;
        }
        if(ring_available_ && x>=ring_x_-135.f &&
           x<=ring_x_-58.f &&
           std::abs(y-(ring_y_+kRingRadius))<84.f){
            mode_=Mode::LoopApproach;speed_=std::clamp(std::abs(vx),400.f,725.f);
            pos_x_=x;pos_y_=y;elapsed_=0.f;++entries_;
            return true;
        }
        return false;
    }

    Position tick(float dt) {
        Position r;
        if(!active()||dt<=0.f)return r;
        dt=std::clamp(dt,0.f,1.f/60.f);
        elapsed_+=dt;
        const float old_x=pos_x_,old_y=pos_y_;
        speed_=std::max(355.f,speed_-29.f*dt);
        switch(mode_){
        case Mode::Serpentine: {
            const float phase=2.f*kPi*(pos_x_-snake_x_)/kSnakePeriod;
            const float dydx=(2.f*kPi*kSnakeAmplitude/kSnakePeriod)*std::sin(phase);
            // Arclength parametrization keeps the character following the
            // drawn snake bend instead of moving at a fixed horizontal speed.
            pos_x_+=speed_*dt/std::sqrt(1.f+dydx*dydx);
            pos_x_=std::min(pos_x_,snake_x_+768.f);
            const float wanted=snake_path_y(pos_x_);
            // No instantaneous 60px vertical teleport on rail entry.
            pos_y_=wanted+(old_y-wanted)*std::max(0.f,1.f-22.f*dt);
            if(pos_x_>=snake_x_+768.f-0.1f){
                mode_=Mode::None;++completions_;r.completed=true;
            }
            break;
        }
        case Mode::LoopApproach: {
            pos_x_+=speed_*dt;
            pos_x_=std::min(pos_x_,ring_x_);
            const float target=ring_y_+kRingRadius;
            pos_y_+=std::clamp(target-pos_y_,-speed_*dt*.7f,speed_*dt*.7f);
            if(pos_x_>=ring_x_-0.1f){
                mode_=Mode::LoopOrbit;angle_=kPi*.5f;
                pos_x_=ring_x_;pos_y_=ring_y_+kRingRadius;
            }
            break;
        }
        case Mode::LoopOrbit: {
            // Bottom -> right -> top -> left -> bottom, a COMPLETE 360°
            // traversal. Uphill/wall/ceiling sensors cannot be simulated by
            // the preview's normal down-only gravity and must not run here.
            angle_-=speed_/kRingRadius*dt;
            pos_x_=ring_x_+kRingRadius*std::cos(angle_);
            pos_y_=ring_y_+kRingRadius*std::sin(angle_);
            if(angle_<=-1.5f*kPi){
                angle_=-1.5f*kPi;pos_x_=ring_x_;pos_y_=ring_y_+kRingRadius;
                mode_=Mode::LoopExit;++orbit_cycles_;
            }
            break;
        }
        case Mode::LoopExit:{
            pos_x_=std::min(ring_x_+156.f,pos_x_+speed_*dt);
            pos_y_=ring_y_+kRingRadius;
            if(pos_x_>=ring_x_+156.f-0.1f){
                mode_=Mode::None;++completions_;r.completed=true;
            }
            break;
        }
        case Mode::None:break;
        }
        min_y_=std::min(min_y_,pos_y_);max_y_=std::max(max_y_,pos_y_);
        r.x=pos_x_;r.y=pos_y_;
        r.vx=(pos_x_-old_x)/dt;r.vy=(pos_y_-old_y)/dt;
        // Preserve tangent velocity; e.g. at loop top vx becomes negative,
        // which must not count as losing the spin dash or leaving the rail.
        return r;
    }
private:
    Mode mode_=Mode::None;
    bool snake_available_=false,ring_available_=false;
    float snake_x_=0.f,snake_y_=0.f,ring_x_=0.f,ring_y_=0.f;
    float speed_=0.f,pos_x_=0.f,pos_y_=0.f,angle_=0.f,elapsed_=0.f;
    float min_y_=1.e8f,max_y_=-1.e8f;
    unsigned entries_=0,completions_=0,orbit_cycles_=0;
};
} // namespace surgepreview
