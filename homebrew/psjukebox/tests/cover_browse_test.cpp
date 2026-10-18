// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/cover_browse.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
int main(){
 const auto middle=jukebox::cover_slot(0.f);
 const auto left=jukebox::cover_slot(-1.f);
 const auto right=jukebox::cover_slot(1.f);
 assert(std::fabs((middle.y+middle.h*.5f)-jukebox::kCoverCenterY)<.01f);
 assert(std::fabs((left.y+left.h*.5f)-jukebox::kCoverCenterY)<.01f);
 assert(std::fabs((right.y+right.h*.5f)-jukebox::kCoverCenterY)<.01f);
 const float gap=right.x-(middle.x+middle.w);
 assert(gap>=20.f&&gap<=35.f);
 assert(std::fabs((middle.x-(left.x+left.w))-gap)<.001f);
 assert(jukebox::allows_live_album_browse(0));
 assert(jukebox::allows_live_album_browse(2)); // currently playing
 assert(jukebox::allows_live_album_browse(1)); // tracklist
 assert(!jukebox::allows_live_album_browse(4)); // settings owns controls
 jukebox::CoverStickEdges sticks;
 assert(sticks.tick(128,128,128,128)==0);
 assert(sticks.tick(0,128,128,128)==-1);
 for(int i=0;i<70;i++)assert(sticks.tick(0,128,128,128)==0);
 assert(sticks.tick(128,128,128,128)==0);
 assert(sticks.tick(128,128,255,128)==1);
 assert(sticks.tick(128,128,128,128)==0);
 assert(sticks.tick(128,255,128,128)==2);
 assert(sticks.tick(128,255,128,128)==0);
 assert(sticks.tick(128,128,128,128)==0);
 assert(sticks.tick(128,128,128,0)==-2);
 assert(sticks.tick(128,128,128,128)==0);
 std::puts("PASS centered covers, 23.5px spacing, live playback shoulders, analog seeking and down tracklist");
}
