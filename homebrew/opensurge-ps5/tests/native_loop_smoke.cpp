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
    constexpr float dt=1.f/120.f;
    unsigned completed=0;
    for(size_t ring=0;ring<map.loop_count();++ring){
        map.reset();
        if(!map.test_setup_loop(ring,900.f)){
            std::fprintf(stderr,"INVALID RING BASE ring=%zu\n",ring);return 5;
        }
        unsigned modes=0;
        bool did_loop=false;
        for(int frame=0;frame<650;++frame){
            map.update(dt,1,false);
            if(map.looping())modes|=1u<<map.movement_mode();
            if(!std::isfinite(map.player_x())||!std::isfinite(map.player_y())){
                std::fprintf(stderr,"INVALID LOOP POS ring=%zu frame=%d\n",ring,frame);
                return 6;
            }
            if(map.loop_completions()>0){did_loop=true;break;}
        }
        if(!did_loop || (modes&15u)!=15u){
            std::fprintf(stderr,"INCOMPLETE LOOP ring=%zu modes=%u complete=%u x=%.1f y=%.1f\n",
              ring,modes,map.loop_completions(),map.player_x(),map.player_y());
            return 7;
        }
        if(map.test_inside()){
            std::fprintf(stderr,"PLAYER EMBEDDED AFTER LOOP ring=%zu\n",ring);return 8;
        }
        ++completed;
        std::printf("PASS ORIGINAL MASK LOOP ring=%zu modes=0x%x x=%.1f y=%.1f\n",
          ring,modes,map.player_x(),map.player_y());
    }
    if(completed<4){
        std::fprintf(stderr,"MISSING AUTHORED LOOPS expected >=4 got %u\n",completed);
        return 9;
    }
    std::printf("PASS MASKED LOOP: rings=%zu four-mode-completions=%u\n",
        map.loop_count(),completed);
    return 0;
}
