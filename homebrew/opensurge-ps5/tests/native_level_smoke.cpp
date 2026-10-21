// Host smoke test for EXACT same C++ native motion/collision class shipped to PS5.
#include "level_preview.hpp"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
int main(int argc,char**argv) {
    if(argc!=2)return 2;
    hui::gfx::Renderer renderer;
    surgepreview::LevelPreview level;
    if(!level.load(renderer,argv[1])){
        std::fprintf(stderr,"LEVEL LOAD FAILED: %s\n",level.error().c_str());
        return 3;
    }
    if(level.count()!=1323 || level.indexed_cells()<100 ||
       level.indexed_cells()>200000){
        std::fprintf(stderr,"BRICK GRID WRONG: bricks=%zu refs=%zu\n",
                     level.count(),level.indexed_cells());
        return 4;
    }
    const float start=level.player_x();
    const auto begin=std::chrono::steady_clock::now();
    // Verify a sustained directional hold, not merely one input pulse.
    for(int i=0;i<220;++i)
        level.update(1.f/120.f,1,false);
    const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now()-begin).count();
    const float moved=level.player_x()-start;
    std::printf("RUN moved=%.1f world px in 1.83s, grid_refs=%zu, host_ms=%lld, grounded=%d\n",
                moved,level.indexed_cells(),static_cast<long long>(elapsed),
                level.grounded()?1:0);
    if(elapsed>5000){std::fprintf(stderr,"COLLISION SIMULATION TOO SLOW\n");return 5;}
    if(moved<30.f || !std::isfinite(level.player_y())){
        std::fprintf(stderr,"MOTION STALLED OR INVALID\n");return 6;
    }
    // Can always recover from terrain/physics state through reset.
    level.reset();
    if(std::abs(level.player_x()-start)>0.01f){
        std::fprintf(stderr,"RESET FAILED\n");return 7;
    }
    hui::gfx::DrawList list;
    level.draw(list);
    if(level.visible()==0){std::fprintf(stderr,"TILES INVISIBLE\n");return 8;}
    std::puts("PASS: original Open Surge level index, 120Hz motion and GPU-free draw path");
    return 0;
}
