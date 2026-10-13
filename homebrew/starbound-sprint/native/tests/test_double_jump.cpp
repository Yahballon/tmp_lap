// SPDX-License-Identifier: GPL-3.0-or-later
// Native-game logic regression tests; no PS5 runtime needed.
#include "game.hpp"
#include <cassert>
#include <iostream>

int main(){
    sprint::Game game;
    game.new_run(0);
    assert(game.grounded);
    assert(game.jumps_remaining()==2);

    // First Cross launch.
    game.step({0,true,false});
    assert(game.velocity.y < -12.f);
    assert(game.jumps_remaining()==1);

    // Holding Cross does not repeatedly jump.
    for(int i=0;i<5;i++)game.step({0,true,false});
    assert(game.jumps_remaining()==1);

    // Cross release, then a second press while airborne.
    game.step({0,false,false});
    assert(!game.grounded);
    game.step({0,true,false});
    assert(game.velocity.y < -11.f);
    assert(game.jumps_remaining()==0);

    // Third press before landing must not reset upward velocity.
    game.step({0,false,false});
    const float before=game.velocity.y;
    game.step({0,true,false});
    assert(game.jumps_remaining()==0);
    assert(game.velocity.y > before);

    // Touching the ground restores both jumps.
    for(int i=0;i<180&&!game.grounded;i++)game.step({0,false,false});
    assert(game.grounded);
    assert(game.jumps_remaining()==2);

    // After landing, the next jump has full first-jump strength.
    game.step({0,true,false});
    assert(game.jumps_remaining()==1);
    assert(game.velocity.y < -12.f);

    // Starting over (also respawn and stage load) restores jump capacity.
    game.new_run(0);
    assert(game.grounded);
    assert(game.jumps_remaining()==2);
    std::cout<<"Double jump regression tests passed\n";
    return 0;
}
