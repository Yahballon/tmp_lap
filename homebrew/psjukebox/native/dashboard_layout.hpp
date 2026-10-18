// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>
namespace jukebox::dashboard {
// Reference layout: 1648x928 screenshot scaled to native 1920x1080.
// Geometry intentionally separated from PS5 GL so it can be regression tested.
struct Rect { float x,y,w,h; };
constexpr Rect header{32,28,1856,134};
constexpr Rect nav{503,58,912,79};
constexpr Rect status{1535,43,337,102};
constexpr Rect artist{34,176,1852,131};
constexpr Rect artistCard{40,321,302,135};
constexpr Rect albumCard{1575,321,303,135};
constexpr Rect player{115,793,1690,174};
constexpr Rect footer{0,982,1920,98};
constexpr Rect playingArtwork{180,810,152,147};
constexpr float coverCenterY=538.f;
constexpr float sideCoverCenterY=565.f;
// Golden focus matches the reference UI independent of album theme hues.
constexpr unsigned gold=0xffc52f;
inline float center_y(float focus){
 return sideCoverCenterY+(coverCenterY-sideCoverCenterY)*
        std::clamp(focus,0.f,1.f);
}
inline float cover_center_x(float position){
 const float distance=std::fabs(position);
 const float compressed=380.f*distance-48.f*std::max(0.f,distance-1.f);
 return 960.f+std::copysign(compressed,position);
}
inline bool intersect(Rect a,Rect b){
 return a.x<b.x+b.w&&b.x<a.x+a.w&&a.y<b.y+b.h&&b.y<a.y+a.h;
}
}
