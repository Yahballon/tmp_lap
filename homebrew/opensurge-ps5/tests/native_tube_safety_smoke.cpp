// Direct regression for REAL original demo underground tube and missing loop gravity.
// v0.5.3 PS5 footage: Tube Out around x4496 emitted 600px/s midair,
// after which Surge fell beneath the original level.
#define SURGEPREVIEW_HOST_TEST 1
#include "level_preview.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
int main(int argc,char** argv){
    if(argc!=2)return 2;
    hui::gfx::Renderer renderer;
    surgepreview::LevelPreview level;
    if(!level.load(renderer,argv[1])){
        std::fprintf(stderr,"failed to load original demo: %s\n",level.error().c_str());
        return 3;
    }
    constexpr float dt=1.f/120.f;
    level.test_put(3728.f,10072.f,1,true);
    level.test_probe(768.f);
    level.test_probe(756.f);
    level.test_pixel(4496.f,10048.f);
    level.test_pixel(4496.f,10072.f);
    level.update(dt,0,false);
    std::printf("TUBE_INITIAL in=%d entries=%u rejected=%u x=%.1f y=%.1f\n",
        int(level.in_tube()),level.tube_entries(),
        level.unsafe_tube_exits(),level.player_x(),level.player_y());
    if(level.in_tube()){
        for(int n=0;n<220&&level.in_tube();++n)
            level.update(dt,0,false);
        if(level.in_tube())return 4;
        if(level.tube_exits() && !level.grounded())return 5;
    }else if(level.unsafe_tube_exits()==0)return 6;
    // Continue running with no player inputs, then also hold RIGHT for a
    // while. Tube endpoints must not turn into endless freefall.
    float deepest=level.player_y();
    for(int i=0;i<300;++i){
        level.update(dt,i>=80?1:0,false);
        deepest=std::max(deepest,level.player_y());
        if(!std::isfinite(level.player_y())||!std::isfinite(level.player_x()))
            return 7;
        if(level.player_x()>3400.f&&level.player_x()<4860.f&&level.player_y()>10390.f){
            std::fprintf(stderr,"FAIL: tube/loop route falls under original Sunshine floor x=%.1f y=%.1f\n",
                level.player_x(),level.player_y());
            return 8;
        }
    }
    std::printf("PASS TUBE SAFETY entries=%u safe_exits=%u refused=%u recovered=%u deepest_y=%.1f final=(%.1f,%.1f)\n",
        level.tube_entries(),level.tube_exits(),level.unsafe_tube_exits(),
        level.loop_safety_recoveries(),deepest,level.player_x(),level.player_y());
    return 0;
}
