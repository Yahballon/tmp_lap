// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/stick_nav.hpp"
#include <cassert>
#include <cstdio>
int main(){
 jukebox::StickNavigator stick;
 assert(stick.tick(128,128,128,128)==0);
 assert(stick.tick(255,128,128,128)==1);
 for(int i=0;i<17;i++)assert(stick.tick(255,128,128,128)==0);
 assert(stick.tick(255,128,128,128)==1);
 assert(stick.tick(128,128,128,128)==0);
 assert(stick.tick(128,128,0,128)==-1);
 assert(stick.tick(128,128,128,128)==0);
 assert(stick.tick(128,0,128,128)==-2);
 assert(stick.tick(128,128,128,128)==0);
 assert(stick.tick(128,128,128,255)==2);
 assert(stick.tick(145,128,128,128)==0);
 assert(stick.tick(128,128,128,128)==0);
 stick.reset();
 assert(stick.tick(128,128,255,128)==1);
 std::puts("PASS: left/right analog sticks, deadzone, repeats and neutral reset");
}
