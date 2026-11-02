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
  // JIT auto mode is explicitly opted into and runs only light-memory profiles.
  s.begin_jit(30);
  assert(s.active && s.total==3 && !s.with_full);
  assert(s.kind==rxbench::AutoSuite::SecureJit3);
  const int expected[3]={7,10,11};
  for(int i=0;i<3;i++){
    assert(s.current_profile()==expected[i]);
    const bool next=s.record(3,true,static_cast<uint64_t>((i+1)*390),30000000);
    assert(s.results[i].profile==expected[i]);
    assert(std::fabs(s.results[i].hps-double((i+1)*13))<0.00001);
    if(i<2)assert(next&&s.active);
    else assert(!next&&s.done&&!s.active);
  }
  s.begin_jit(10);
  assert(!s.record(5,false,0,0));
  assert(s.recorded==1 && s.done && !s.active);
  s.begin_jit(10);
  assert(s.record(3,true,1,1000000));
  s.cancel();
  assert(!s.record(4,false,1,1000000));
  assert(s.cancelled && s.recorded==2);
  s.begin_all(30);
  const int expected_all[8]={0,1,2,3,7,10,11,15};
  assert(s.kind==rxbench::AutoSuite::Safe8&&s.total==8&&!s.with_full);
  for(int i=0;i<8;i++){
    assert(s.current_profile()==expected_all[i]);
    bool next=s.record(3,true,100,30000000);
    assert(s.results[i].profile==expected_all[i]);
    if(i<7)assert(next);else assert(!next&&s.done);
  }
  s.begin_all(10);
  for(int i=0;i<3;i++)assert(s.record(3,true,10,1000000));
  assert(!s.record(5,false,0,0));
  assert(s.recorded==4&&s.done&&!s.active);
  s.begin_all(30);
  assert(s.record(3,true,10,1000000));
  s.cancel();
  assert(!s.record(4,false,0,0)&&s.cancelled);
  // Original 4-stage interpreter plan remains available after reset.
  s.begin(false,30);
  assert(s.current_profile()==0 && s.kind==rxbench::AutoSuite::Light4);
  std::puts("PASS: AUTO SAFE8 with final 64 MiB diagnostic; legacy AUTO4/5; JIT; cancel/failure");
}
