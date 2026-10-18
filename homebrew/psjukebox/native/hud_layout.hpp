// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>
namespace jukebox::hud {
// 1920 x 1080 native PS5 UI layout, adapted proportionally from the user's
// 1648 x 928 reference screenshot. All measurements are in virtual pixels.
struct Rect {float x,y,w,h;};
constexpr Rect header={32,28,1856,136};
constexpr Rect tabs={504,59,917,80};
constexpr Rect status={1536,46,343,101};
constexpr Rect artistHero={34,179,1852,128};
constexpr Rect browseCard={42,326,310,156};
constexpr Rect albumCard={1581,326,306,156};
constexpr Rect player={116,795,1688,171};
constexpr Rect playerThumb={182,811,152,139};
constexpr Rect footer={0,986,1920,94};
constexpr float carouselCenterX=960.f;
constexpr float carouselCenterY=525.f;
constexpr float sleeveSpacing=380.f;
constexpr unsigned coverSlots=5;
constexpr unsigned equalizerBars=23;
constexpr Rect selectedCover={762.5f,317.f,395.f,416.f};
inline float reflectionHeight(float coverBottom){
 return std::clamp(player.y-coverBottom-13.f,0.f,56.f);
}
inline float visualizerBar(unsigned bar,float seconds,bool paused){
 const float center=1.f-std::abs((int)bar-11)/11.f;
 const float wave=std::abs(std::sin(seconds*3.4f+float(bar)*.79f));
 return paused?4.f:5.f+(7.f+center*40.f)*(.28f+.72f*wave);
}
static_assert(player.y>selectedCover.y+selectedCover.h);
static_assert(footer.y>player.y+player.h);
static_assert(header.x+header.w<=1920.f);
static_assert(status.x>tabs.x+tabs.w);
}
