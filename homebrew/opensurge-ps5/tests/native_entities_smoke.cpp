// Host regression of the ACTUAL PS5 native entity system with upstream assets.
// No surrogate/gameplay mock: the same level_entities.hpp ships in eboot.bin.
#include "level_entities.hpp"
#include <cassert>
#include <cstdio>
int main(int argc,char** argv){
    if(argc!=2)return 2;
    hui::gfx::Renderer render;
    surgepreview::LevelEntities entities;
    if(!entities.load(render,argv[1])){
        std::fprintf(stderr,"ENTITY LOAD FAILED %s\n",entities.error().c_str());
        return 3;
    }
    if(entities.count()!=128) {
        std::fprintf(stderr,"ORIGINAL ENTITY COUNT changed %zu\n",entities.count());
        return 4;
    }
    // Authentic upstream demo: Collectible @ (256, 9808).
    auto result=entities.tick(1.f/120.f,256.f,9828.f,0.f);
    assert(result.collected==1 && entities.collected()==1);
    // A ring may NEVER count twice, even when touched on consecutive frames.
    for(int i=0;i<120;++i) {
        result=entities.tick(1.f/120.f,256.f,9828.f,0.f);
        assert(result.collected==0);
    }
    // Standard spring @ 2928, 9696 and stronger @ 8144, 10080.
    result=entities.tick(1.f/120.f,2928.f,9696.f,30.f);
    assert(result.spring_power==600.f);
    result=entities.tick(1.f/120.f,8144.f,10080.f,30.f);
    assert(result.spring_power==960.f);
    // A spring must never react repeatedly to stationary contact; old 0.3.0
    // console log showed many consecutive BOUNCE messages around x=2928.
    for(int i=0;i<100;++i){
        result=entities.tick(1.f/120.f,8144.f,10080.f,30.f);
        assert(result.spring_power==0.f);
    }
    // Leave its collision range to rearm, then land on it once again.
    for(int i=0;i<80;++i)
        entities.tick(1.f/120.f,8300.f,10080.f,30.f);
    result=entities.tick(1.f/120.f,8144.f,10080.f,30.f);
    assert(result.spring_power==960.f);
    // Spring does not trigger on upward velocity.
    result=entities.tick(1.f/120.f,8144.f,10080.f,-250.f);
    assert(result.spring_power==0.f);
    // Goal from original demo: (11776,10848).
    result=entities.tick(1.f/120.f,11776.f,10848.f,0.f);
    assert(result.goal && entities.complete());
    result=entities.tick(1.f/120.f,11776.f,10848.f,0.f);
    assert(!result.goal);
    hui::gfx::DrawList ui;
    entities.draw(ui,11776.f,10848.f);
    assert(entities.visible()>0);
    entities.reset();
    assert(entities.collected()==0 && !entities.complete());
    result=entities.tick(1.f/120.f,256.f,9828.f,0.f);
    assert(result.collected==1);
    std::printf("PASS NATIVE ENTITIES original count=%zu, pickup-once, 600/960 spring, goal and reset\n",
                entities.count());
}
