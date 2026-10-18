// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/hud_layout.hpp"
#include "../native/cover_browse.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
int main(){
 using namespace jukebox::hud;
 assert(header.y<header.h+header.y&&header.w>1800);
 assert(tabs.x>header.x&&status.x>tabs.x+tabs.w);
 assert(artistHero.y>=header.y+header.h);
 assert(browseCard.x<carouselCenterX&&albumCard.x>carouselCenterX);
 assert(player.y>=selectedCover.y+selectedCover.h+60);
 assert(player.w>1600&&playerThumb.x>player.x);
 assert(footer.y>=player.y+player.h);
 const auto center=jukebox::cover_slot(0);
 const auto near=jukebox::cover_slot(-1);
 assert(std::fabs(center.x-selectedCover.x)<1.0f);
 assert(std::fabs(center.y-selectedCover.y)<1.0f);
 assert(near.x+near.w<center.x);
 assert(reflectionHeight(center.y+center.h)>0);
 assert(reflectionHeight(player.y)>-0.01f);
 for(unsigned i=0;i<equalizerBars;i++){
  const float active=visualizerBar(i,.7f,false);
  assert(active>=5&&active<=52);
  assert(visualizerBar(i,.7f,true)==4);
 }
 std::puts("PASS PSJukebox reference dashboard geometry, coverflow, mini-player, visualizer");
}
