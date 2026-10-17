// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>
namespace jukebox {
// 0.48s eased physical coverflow. A new step cannot be queued before the
// previous transition finishes; holding a button cannot skip multiple albums.
class CoverMotion {
public:
 static constexpr float duration=0.48f;
 bool begin(int direction){
  if(active_||(direction!=1&&direction!=-1))return false;
  direction_=direction;
  elapsed_=0;
  active_=true;
  return true;
 }
 void tick(float seconds){
  if(!active_)return;
  elapsed_=std::min(duration,elapsed_+std::clamp(seconds,0.f,0.08f));
  if(elapsed_>=duration){active_=false;direction_=0;}
 }
 float offset()const{
  if(!active_)return 0.f;
  const float p=elapsed_/duration;
  // smoothstep has zero velocity at both endpoints.
  const float eased=p*p*(3.f-2.f*p);
  return (float)direction_*(1.f-eased);
 }
 float progress()const{return active_?elapsed_/duration:1.f;}
 bool active()const{return active_;}
 void reset(){elapsed_=duration;direction_=0;active_=false;}
private:
 int direction_=0;
 float elapsed_=duration;
 bool active_=false;
};
}
