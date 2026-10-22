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
        for(int direction:{1,-1}){
            map.reset();
            if(!map.test_setup_loop(ring,900.f*float(direction))){
                std::fprintf(stderr,"INVALID RING BASE ring=%zu dir=%d\n",ring,direction);
                return 5;
            }
            unsigned modes=0;
            bool did_loop=false;
            for(int frame=0;frame<650;++frame){
                map.update(dt,direction,false);
                if(map.looping())modes|=1u<<map.movement_mode();
                if(!std::isfinite(map.player_x())||!std::isfinite(map.player_y())){
                    std::fprintf(stderr,"INVALID LOOP POS ring=%zu dir=%d frame=%d\n",
                        ring,direction,frame);
                    return 6;
                }
                if(map.loop_completions()>0){did_loop=true;break;}
            }
            if(!did_loop || (modes&15u)!=15u){
                std::fprintf(stderr,"INCOMPLETE LOOP ring=%zu dir=%d modes=%u complete=%u x=%.1f y=%.1f\n",
                  ring,direction,modes,map.loop_completions(),map.player_x(),map.player_y());
                return 7;
            }
            if(map.test_inside()){
                std::fprintf(stderr,"PLAYER EMBEDDED ring=%zu dir=%d\n",ring,direction);
                return 8;
            }
            ++completed;
            std::printf("PASS ORIGINAL MASK LOOP ring=%zu direction=%d modes=0x%x x=%.1f y=%.1f\n",
              ring,direction,modes,map.player_x(),map.player_y());
        }
    }
    if(completed<8){
        std::fprintf(stderr,"MISSING ORIGINAL LOOP TESTS expected >=8 got %u\n",completed);
        return 9;
    }
    // Firmware 5.10 footage at ~6.5s: player was rolling RIGHT at
    // x=5689, feet=9922, VX~597 immediately before the x5888 ring.
    // Do not pass this regression by teleporting to the ring center.
    map.reset();
    // The actual demo engages Layer Green at x5744 on approach.
    // Unlike the synthetic ring test, retain original scripted layer events.
    map.test_enable_routes(true);
    map.test_put(5689.f,9922.f,1,true);
    map.test_set_rolling_speed(597.f);
    bool video_entered=false,video_complete=false;
    unsigned video_modes=0;
    for(int frame=0;frame<500;++frame){
        map.update(dt,1,false);
        if(map.looping()){
            video_entered=true;
            video_modes|=1u<<map.movement_mode();
        }
        if(map.loop_completions()>0){
            video_complete=true;break;
        }
    }
    if(!video_entered || !video_complete || video_modes!=15u){
        std::fprintf(stderr,"FAIL PS5 VIDEO LOOP at x5689,9922: entered=%d completed=%d modes=%u final=(%.1f,%.1f) blocked=%u\n",
            video_entered,video_complete,video_modes,map.player_x(),map.player_y(),
            map.collision_blocks());
        return 10;
    }
    std::puts("PASS PS5 VIDEO LOOP: rolling at x5689,9922 enters and completes original ring");
    std::printf("PASS MASKED LOOP: rings=%zu two-way-four-mode-completions=%u\n",
        map.loop_count(),completed);
    return 0;
}
