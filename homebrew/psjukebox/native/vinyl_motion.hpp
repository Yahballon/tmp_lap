// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>
namespace jukebox {
// Time-based 33 1/3 RPM LP motor. Rendering and audio stay independent.
class VinylMotion {
public:
 static constexpr float kPi=3.14159265358979323846f;
 static constexpr float kRadiansPerSecond=2.f*kPi*(33.333333f/60.f);
 void tick(float dt,bool isPlaying){
  if(!isPlaying)return;
  angle_=std::fmod(angle_+std::clamp(dt,0.f,.05f)*kRadiansPerSecond,2.f*kPi);
 }
 float angle()const{return angle_;}
 void reset(){angle_=0.f;}
private:
 float angle_=0.f;
};
