// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
namespace jukebox {
class StickNavigator {
 int direction_=0;
 int wait_=0;
 static int axis(uint8_t v) {
  if(v<=54)return -1;
  if(v>=200)return 1;
  return 0;
 }
public:
 // -1: left; +1: right; -2: up; +2: down; 0: no navigation.
 int tick(uint8_t lx,uint8_t ly,uint8_t rx,uint8_t ry){
  const int lhorizontal=axis(lx),lvertical=axis(ly);
  const int rhorizontal=axis(rx),rvertical=axis(ry);
  const int dir=lhorizontal?lhorizontal:(lvertical?2*lvertical:
                 (rhorizontal?rhorizontal:2*rvertical));
  if(!dir){direction_=wait_=0;return 0;}
  if(dir!=direction_){direction_=dir;wait_=18;return dir;}
  if(--wait_<=0){wait_=8;return dir;}
  return 0;
 }
 void reset(){direction_=wait_=0;}
};
} // namespace jukebox
