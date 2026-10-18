// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/vinyl_motion.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
int main(){
 jukebox::VinylMotion v;
 assert(v.angle()==0.f);
 v.tick(.016f,false);assert(v.angle()==0.f);
 v.tick(.05f,true);const float angle=v.angle();assert(angle>0.f);
 v.tick(.05f,false);assert(std::fabs(v.angle()-angle)<1e-6f);
 for(int i=0;i<3000;i++)v.tick(.016f,true);
 assert(v.angle()>=0.f&&v.angle()<2.f*jukebox::VinylMotion::kPi);
 v.reset();assert(v.angle()==0.f);
 puts("PASS: LP rotates during playback, holds on pause, wraps without overflow");
}
