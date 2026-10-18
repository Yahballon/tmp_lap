// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/dashboard_layout.hpp"
#include "../native/cover_browse.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
int main(){
 using namespace jukebox::dashboard;
 assert(header.x>0&&header.y>0&&header.x+header.w<1920);
 assert(nav.x>header.x&&nav.x+nav.w<status.x);
 assert(status.x>nav.x+nav.w&&status.x+status.w<1920);
 assert(artistCard.y>header.y+header.h);
 assert(albumCard.y==artistCard.y);
 assert(player.y>artistCard.y+artistCard.h);
 assert(artistCard.y<267); // no full-width artist context strip
 assert(coverCenterY<510.f);
 assert(footer.y>=player.y+player.h);
 assert(!intersect(artistCard,albumCard));
 assert(playingArtwork.x>player.x&&playingArtwork.y>player.y);
 assert(playingArtwork.x+playingArtwork.w<player.x+player.w);
 assert(playingArtwork.y+playingArtwork.h<player.y+player.h);
 assert(gold==0xffc52f);
 assert(std::abs(cover_center_x(0)-960.f)<.01f);
 assert(cover_center_x(-2.f)>0&&cover_center_x(2.f)<1920.f);
 assert(std::abs(cover_center_x(-1.f)+cover_center_x(1.f)-1920.f)<.01f);
 assert(center_y(1.f)<center_y(0.f));
 const auto center=jukebox::cover_slot(0.f);
 const auto side=jukebox::cover_slot(-1.f);
 assert(center.y+center.h*.5f<side.y+side.h*.5f);
 assert(side.x>artistCard.x+artistCard.w);
 assert(center.y+center.h<player.y); // room for reflections
 puts("PASS dashboard reference geometry, spacing, centered covers and separate now-playing");
}
