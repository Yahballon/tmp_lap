// SPDX-License-Identifier: GPL-3.0-or-later
#include "game.hpp"
#include <cassert>
#include <iostream>
int main(){
    sprint::Game game;
    for(int stage=0;stage<sprint::kStages;++stage){
        // Progress unlock restriction is respected; force initial selection via test fixture.
        game.progress.unlocked=stage;
        game.new_run(stage);
        assert(game.stage==stage);
        assert(game.grounded);
        assert(game.jumps_remaining()==2);
        assert(!game.pickups.empty());
        assert(!game.drones.empty());
        assert(game.solid_at(0,sprint::kRows-1));
        assert(!game.solid_at(0,sprint::kRows-2));
        assert(game.finish_x()<sprint::kColumns*sprint::kTile);
        for(int i=0;i<2;i++)game.step({1.f,false,true});
        assert(game.phase==sprint::Phase::playing);
        if(stage%4==1||stage%4==3)assert(!game.moving_platforms.empty());
    }
    std::cout<<"All world structure checks passed\n";
}
