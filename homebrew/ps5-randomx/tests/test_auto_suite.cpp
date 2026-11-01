// SPDX-License-Identifier: GPL-3.0-or-later
// Host-side test of the automatic benchmark sequencer.
#include "auto_suite.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>

int main(){
  rxbench::AutoSuite s;
  s.begin(false,30);
  assert(s.active && !s.done && s.total==4 && s.step==0 && !s.with_full);
  for(int i=0;i<4;i++){
    const bool next=s.record(3,true,static_cast<uint64_t>(60*(i+1)),30000000);
    assert(s.results[i].profile==i);
    assert(std::fabs(s.results[i].hps-2.0*(i+1))<0.00001);
    assert(s.recorded==i+1);
    if(i!=3){assert(next&&s.active&&s.step==i+1);}
    else {assert(!next&&!s.active&&s.done);}
  }
  assert(!s.record(3,true,1,1)); // can't collect stale completion twice
  s.begin(true,10);
  assert(s.total==5 && s.with_full && !s.done);
  for(int i=0;i<5;i++){
    const bool next=s.record(3,true,static_cast<uint64_t>(i+1),1000000);
    if(i<4)assert(next);else assert(!next&&s.done);
  }
  s.begin(false,30);
  s.record(3,true,60,30000000);
  s.cancel();
  assert(!s.record(4,false,10,1000000));
  assert(s.done && s.cancelled && s.recorded==2 && !s.active);
  s.begin(true,30);
  assert(!s.record(5,false,0,0));
  assert(s.recorded==1 && s.done && !s.active);
  std::puts("PASS: four-profile auto-run, opt-in five-profile run, cancel and failure");
}
