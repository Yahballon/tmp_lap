// Looping regression against the original pinned Open Surge collision mask.
// Native MM_* compatibility prototype, not the original physicsactor.c.
#define SURGEPREVIEW_HOST_TEST 1
#include "level_preview.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
int main(int argc,char** argv){
    if(argc!=2)return 2;
    hui::gfx::Renderer renderer;
    surgepreview::LevelPreview map;
    if(!map.load(renderer,argv[1]))return 3;
    map.test_enable_routes(false);
    if(!map.loop_mode_ready()||map.loop_count()<1){
        std::fputs("Missing original masked Sunshine loop\n",stderr);return 4;
    }
    if(!map.test_setup_loop(0,900.f)){
        std::fputs("Could not place actor at masked original ring base\n",stderr);return 5;
    }
    constexpr float dt=1.f/120.f;
    unsigned modes=0;
    bool did_loop=false;
    for(int frame=0;frame<650;++frame){
        map.update(dt,1,false);
        if(map.looping())modes|=1u<<map.movement_mode();
        if(!std::isfinite(map.player_x())||!std::isfinite(map.player_y())){
            std::fprintf(stderr,"INVALID LOOP POS frame=%d\n",frame);
            return 6;
        }
        if(map.loop_completions()>0){did_loop=true;break;}
    }
    if(!did_loop || (modes&15u)!=15u){
        std::fprintf(stderr,"INCOMPLETE LOOP ring=%zu modes=%u completed=%u x=%.1f y=%.1f\n",
          map.loop_count(),modes,map.loop_completions(),map.player_x(),map.player_y());
        return 7;
    }
    if(map.test_inside()){
        std::fputs("PLAYER EMBEDDED AFTER LOOP\n",stderr);return 8;
    }
    std::printf("PASS MASKED LOOP: rings=%zu orientations=0x%x complete=%u x=%.1f y=%.1f\n",
        map.loop_count(),modes,map.loop_completions(),map.player_x(),map.player_y());
    return 0;
}
