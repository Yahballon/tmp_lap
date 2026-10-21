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
    // Regression based on the PS5 2026-10-21 recording: user held RIGHT only,
    // FPS remained 59.6 but Surge repeatedly stopped at x ~723 and x ~900.
    // This longer no-jump run has to cross the slope and reach the map edge.
    float farthest=level.player_x();
    for(int i=0;i<900;++i){
        level.update(1.f/120.f,1,false);
        farthest=std::max(farthest,level.player_x());
        if(!std::isfinite(level.player_x())||!std::isfinite(level.player_y()))return 9;
    }
    std::printf("RIGHT_ONLY max_x=%.1f (target>1000), blocked=%u auto_respawns=%u\n",
                farthest,level.collision_blocks(),level.auto_respawns());
    if(farthest<1000.f){
        std::fprintf(stderr,"FAIL right-only held: character stuck at terrain slope or ledge\n");
        return 10;
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
