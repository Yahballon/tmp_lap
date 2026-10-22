// Regression: original Open Surge demo-1.lev layered brick collision + 6 tube pairs.
// This is a native compatibility subset, NOT the original SurgeScript runtime.
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
        std::fprintf(stderr,"ROUTE LEVEL LOAD %s\n",level.error().c_str());
        return 3;
    }
    assert(level.route_count()>50);
    assert(level.active_layer()==0);
    constexpr float dt=1.f/120.f;
    // Real original Layer Green entity (3696,9552).
    level.test_put(3696.f,9576.f);
    level.update(dt,0,false);
    assert(level.active_layer()==1 && level.layer_switches()>=1);
    // Real original Layer Yellow entity (3792,9392).
    level.test_put(3792.f,9416.f);
    level.update(dt,0,false);
    assert(level.active_layer()==2 && level.layer_switches()>=2);

    // A walking player must not be auto-transported just because a tube
    // trigger exists there. The original Tube In requires rolling gameplay.
    level.reset();
    level.test_put(3728.f,10072.f,1,false);
    level.update(dt,0,false);
    assert(!level.in_tube() && level.tube_entries()==0);

    // Enter original Tube In 3728,10048, head right to Tube Out 4496,10048.
    level.reset();
    level.test_put(3728.f,10072.f,1,true);
    level.update(dt,0,false);
    assert(level.in_tube() && level.tube_entries()==1);
    for(int i=0;i<200 && level.tube_exits()==0;++i)level.update(dt,0,false);
    assert(!level.in_tube() && level.tube_exits()==1);
    assert(std::abs(level.player_x()-4496.f)<3.f);
    assert(std::abs(level.velocity_x())>=590.f);
    // Reverse route from 4464 Tube In to 3696 Tube Out.
    level.reset();
    level.test_put(4464.f,10072.f,-1,true);
    level.update(dt,0,false);
    assert(level.in_tube());
    for(int i=0;i<200 && level.tube_exits()==0;++i)level.update(dt,0,false);
    assert(level.tube_exits()==1 && level.player_x()<3720.f);

    std::printf("PASS AUTHORED ROUTES: %zu original layer/tube entities, green/yellow layer switching, two-way 600px/s Tube In/Out\n",level.route_count());
    if(level.loop_mode_ready()){
        std::fputs("Unexpectedly advertised full loop physics\n",stderr);return 4;
    }
    std::puts("LOOP PHYSICS NOT IMPLEMENTED: original four-mode obstacle sensors still required");
    return 0;
}
