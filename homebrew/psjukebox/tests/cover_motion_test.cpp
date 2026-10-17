// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/cover_motion.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
int main(){
 jukebox::CoverMotion x;
 assert(!x.active()&&x.offset()==0.f);
 assert(!x.begin(0)&&!x.begin(2));
 assert(x.begin(1)&&x.active()&&x.offset()==1.f);
 assert(!x.begin(-1)); // No skipped albums during slide.
 float last=x.offset();
 for(int i=0;i<20;i++){
  x.tick(.016f);
  assert(x.offset()<=last+0.000001f);
  last=x.offset();
 }
 assert(x.active());
 for(int i=0;i<40;i++)x.tick(.016f);
 assert(!x.active()&&x.offset()==0.f);
 assert(x.begin(-1)&&x.offset()==-1.f);
 for(int i=0;i<15;i++)x.tick(.016f);
 assert(std::fabs(x.offset()+.5f)<.001f);
 for(int i=0;i<15;i++)x.tick(.016f);
 assert(!x.active()&&x.offset()==0.f);
 x.begin(1);x.reset();assert(!x.active()&&x.offset()==0.f);
 std::puts("PASS eased coverflow, one-step navigation, duration, reset, both directions");
}
