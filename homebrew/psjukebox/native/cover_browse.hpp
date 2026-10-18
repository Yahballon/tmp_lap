// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace jukebox {
constexpr float kCoverCenterX=960.f;
constexpr float kCoverCenterY=525.f;
constexpr float kCoverSpacing=380.f;
struct CoverSlot {float x,y,w,h;};
inline CoverSlot cover_slot(float position){
 const float focus=std::clamp(1.f-std::fabs(position),0.f,1.f);
 const float w=318.f+77.f*focus;
 const float h=320.f+96.f*focus;
 return {kCoverCenterX+position*kCoverSpacing-w*.5f,
         kCoverCenterY-h*.5f,w,h};
}
// Active playback must not disable artist/album browsing. All these screens
// can return to the album carousel without stopping the current decoder.
inline bool allows_live_album_browse(int screen){
 return screen==0||screen==1||screen==2||screen==5;
}
// Only D-pad left/right navigate album covers. R2/L2 now select actual
// audio tracks, crossing into another album at song boundaries.
inline int album_browse_step(bool dpadLeft,bool dpadRight){
 if(dpadLeft)return -1;
 if(dpadRight)return 1;
 return 0;
}
class CoverStickEdges {
 int direction_=0;
 static int axis(uint8_t value){
  if(value<60)return -1;
  if(value>195)return 1;
  return 0;
 }
public:
 // -1 seek backward, 1 seek forward, -2 header, 2 tracklist.
 // Emits only once per stick deflection; must return to neutral to rearm.
 int tick(uint8_t lx,uint8_t ly,uint8_t rx,uint8_t ry){
  const int horizontal=axis(lx)?axis(lx):axis(rx);
  const int vertical=axis(ly)?axis(ly):axis(ry);
  const int next=horizontal?horizontal:vertical?2*vertical:0;
  if(next==0){direction_=0;return 0;}
  if(next==direction_)return 0;
  direction_=next;
  return next;
 }
 void reset(){direction_=0;}
};
}
