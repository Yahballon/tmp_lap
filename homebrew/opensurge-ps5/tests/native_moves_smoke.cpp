// Real native level C++ regression with authentic original Open Surge level.
#include "level_preview.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>

int main(int argc,char** argv){
    if(argc!=2)return 2;
    hui::gfx::Renderer renderer;
    surgepreview::LevelPreview level;
    if(!level.load(renderer,argv[1]))return 3;
    constexpr float dt=1.f/120.f;

    // Crouch (authentic Surge animation 4 final sprite #39).
    level.update(dt,0,false,true,false);
    assert(level.crouching() && level.sprite_frame()==39);
    level.update(dt,0,false,false,true);
    assert(level.looking_up() && level.sprite_frame()==36);

    // DOWN + X must charge, never initiate the usual jump.
    level.update(dt,0,true,true,false);
    assert(level.charging() && level.grounded());
    assert(std::abs(level.velocity_x())<0.01f);
    assert(level.sprite_frame()==20 || level.sprite_frame()==22 ||
           level.sprite_frame()==24 || level.sprite_frame()==26);
    for(int i=0;i<15;++i)level.update(dt,0,false,true,false);
    for(int i=0;i<4;++i){
        level.update(dt,0,true,true,false);
        for(int k=0;k<3;++k)level.update(dt,0,false,true,false);
    }
    assert(level.dash_charge()>0.7f);
    // Releasing DOWN preserves the prior-facing direction and launches spin.
    const float before=level.player_x();
    level.update(dt,0,false,false,false);
    assert(level.dash_releases()==1 && level.rolling() && !level.charging());
    const float launch=level.velocity_x();
    assert(launch>600.f && launch<=720.f);
    assert(level.sprite_frame()>=19 && level.sprite_frame()<=26);
    for(int i=0;i<60;++i)level.update(dt,0,false,false,false);
    const float after=level.player_x();
    std::printf("SPINDASH charge x=%.1f launch vx=%.1f moved=%.1f rolling=%d\n",
       before,launch,after-before,level.rolling()?1:0);
    assert(after-before>65.f); // should cover ground, not remain frozen
    assert(level.dash_releases()==1);

    // RESET needs to clear all buffered move state (and preserve collision).
    level.reset();
    assert(!level.charging() && !level.rolling() && level.dash_releases()==0);
    assert(level.grounded());

    // Ordinary jump must remain the previously working behavior.
    level.update(dt,0,true,false,false);
    assert(!level.grounded() && level.velocity_y()<0.f);
    assert(!level.charging());

    // Move right as in confirmed working v0.2.4 regression, then roll via DOWN.
    level.reset();
    for(int i=0;i<100;++i)level.update(dt,1,false,false,false);
    if(level.grounded() && std::abs(level.velocity_x())>=60.f){
        level.update(dt,1,false,true,false);
        assert(level.rolling());
    }

    std::puts("PASS native original-art roll, crouch, look-up, charge/release Spin Dash, jump and reset");
}
