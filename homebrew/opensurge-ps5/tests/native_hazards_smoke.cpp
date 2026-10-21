// Native smoke test using real demo-1.lev hazard positions and upstream art.
#include "level_hazards.hpp"
#include <cassert>
#include <cstdio>
int main(int argc,char** argv){
    if(argc!=2)return 2;
    hui::gfx::Renderer renderer;
    surgepreview::LevelHazards world;
    if(!world.load(renderer,argv[1])){
        std::fprintf(stderr,"HAZARD LOAD FAILED %s\n",world.error().c_str());
        return 3;
    }
    assert(world.count()==13 && world.lives()==3);
    constexpr float dt=1.f/120.f;
    // Mosquito @ (7904,10576): an attack destroys the enemy and gives points.
    auto e=world.tick(dt,7904.f,10596.f,-130.f,true,0);
    assert(e.killed==1 && !e.hit && world.defeated()==1 && world.score()==100);
    e=world.tick(dt,7904.f,10596.f,-130.f,true,0);
    assert(e.killed==0); // one kill only
    // Spikes @ (1616,10080): ring loss first, not loss of life.
    e=world.tick(dt,1616.f,10080.f,0.f,false,5);
    assert(e.hit && !e.lost_life && world.lives()==3);
    // Invulnerability frames must prevent chain-hit until timer expires.
    for(int i=0;i<150;++i){
        e=world.tick(dt,1616.f,10080.f,0.f,false,0);
        assert(!e.hit);
    }
    for(int i=0;i<100;++i)world.tick(dt,0.f,0.f,0.f,false,0);
    e=world.tick(dt,1616.f,10080.f,0.f,false,0);
    assert(e.hit && e.lost_life && world.lives()==2);
    // Resetting entities on respawn MUST NOT restore the lost life.
    world.restart_level();
    assert(world.lives()==2 && world.defeated()==1);
    world.reset_game();
    assert(world.lives()==3 && world.score()==0 && world.defeated()==0);
    hui::gfx::DrawList ui;
    world.draw(ui,1616.f,10080.f);
    std::printf("PASS NATIVE HAZARDS 13 authentic entities, mosquito kill, spike damage, shield window, lives, restart\n");
}
