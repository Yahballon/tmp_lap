// Native authored Sunshine curve regression: test with ORIGINAL demo-1 map.
#define SURGEPREVIEW_HOST_TEST 1
#include "level_preview.hpp"
#include "level_curves.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
int main(int argc,char** argv){
    if(argc!=2)return 2;
    constexpr float dt=1.f/120.f;
    hui::gfx::Renderer renderer;
    surgepreview::LevelPreview level;
    if(!level.load(renderer,argv[1])){
        std::fprintf(stderr,"LEVEL_CURVES_LOAD_FAIL %s\n",level.error().c_str());return 3;
    }
    level.test_enable_routes(false); // test individual physical rails
    if(!level.loop_mode_ready()){
        std::fprintf(stderr,"CURVES_MISSING exact Sunshine 50/53/41/42 original bricks\n");return 4;
    }
    // Real PS5 user video: x ~3828,y10064, roll straight through snake
    // rather than following S-shaped channel between huge solid blocks.
    for(float x:{3712.f,3776.f,3840.f,3904.f,3968.f,4032.f,4096.f,4224.f,4352.f})
        level.test_scan_column(x,9960,10170,8);
    for(float x:{5776.f,5824.f,5888.f,5952.f,6000.f})
        level.test_scan_column(x,9670,9950,8);
    level.test_put(3704.f,10048.f,1,true);
    unsigned entered=0;
    float miny=1e6f,maxy=-1e6f;
    unsigned snake_in_solid=0,snake_sampled=0;
    bool done=false;
    for(int i=0;i<900;++i){
        level.update(dt,1,false,true,false);
        if(level.in_curve()){
            ++entered;
            ++snake_sampled;
            if(level.test_inside())++snake_in_solid;
            miny=std::min(miny,level.player_y());
            maxy=std::max(maxy,level.player_y());
        }
        if(level.curve_completions()){
            done=true;break;
        }
    }
    std::printf("SERPENTINE_RESULT x=%.1f y=%.1f range=%.1f..%.1f ticks=%u entries=%u completed=%u\n",
        level.player_x(),level.player_y(),miny,maxy,entered,
        level.curve_entries(),level.curve_completions());
    std::printf("CURVE_MASK snake_inside=%u of %u frames\n",snake_in_solid,snake_sampled);
    if(!done||entered<110||level.player_x()<4480.f||
       miny>10048.f||maxy<10105.f||maxy-miny<65.f)return 5;
    if(level.in_curve())return 6;
    // Screenshot original loop tile: 41 @ 5760,9664 and 42 @ 5888,9664.
    // Enter at bottom from the LEFT, then travel bottom->right->top->left
    // ->bottom. The old downward-only gravity wrongly threw us out at top.
    level.test_put(5765.f,9923.f,1,true);
    int orbit_ticks=0,positive=0,negative=0;
    unsigned ring_in_solid=0,ring_sampled=0;
    float mx=1e6f,Mx=-1e6f,my=1e6f,My=-1e6f;
    bool circle_enter=false,circle_finished=false,exited_before_orbit=false;
    const unsigned complete_before=level.curve_completions();
    const unsigned orbits_before=level.curve_orbits();
    for(int i=0;i<900;++i){
        level.update(dt,1,false,true,false);
        if(level.in_curve()){
            circle_enter=true;
            ++ring_sampled;
            if(level.test_inside())++ring_in_solid;
            mx=std::min(mx,level.player_x());Mx=std::max(Mx,level.player_x());
            my=std::min(my,level.player_y());My=std::max(My,level.player_y());
            if(level.curve_mode()==3){
                ++orbit_ticks;
                if(level.velocity_x()>125.f)++positive;
                if(level.velocity_x()<-125.f)++negative;
            }
        }else if(circle_enter && level.curve_orbits()==orbits_before){
            exited_before_orbit=true;break;
        }
        if(level.curve_completions()>complete_before){
            circle_finished=true;break;
        }
    }
    std::printf("LOOP_360_RESULT x=%.1f y=%.1f orbit_ticks=%d range_x=%.1f..%.1f range_y=%.1f..%.1f positive=%d negative=%d turns=%u completed=%u early_exit=%d\n",
        level.player_x(),level.player_y(),orbit_ticks,mx,Mx,my,My,
        positive,negative,level.curve_orbits(),level.curve_completions(),exited_before_orbit?1:0);
    std::printf("CURVE_MASK ring_inside=%u of %u frames\n",ring_in_solid,ring_sampled);
    if(!circle_enter||!circle_finished||exited_before_orbit||
       orbit_ticks<150||level.curve_orbits()!=orbits_before+1||
       level.player_x()<6035.f||mx>5810.f||Mx<5980.f||
       my>9740.f||My<9910.f||negative<35||positive<35)return 7;
    // Underpowered regular walking does NOT auto-teleport into the loop.
    level.test_put(5765.f,9923.f,1,false);
    for(int i=0;i<10;++i)level.update(dt,1,false,false,false);
    if(level.in_curve())return 8;
    level.reset();
    if(level.in_curve()||level.curve_entries()!=0||level.curve_completions()!=0)return 9;
    std::puts("PASS native Sunshine snake follows all S bends, 360 loop completes, early escape prevented, no walking teleport, reset safe");
}
