// SPDX-License-Identifier: GPL-3.0-or-later
#include "dualsense_zplayer.hpp"
#include <cassert>
#include <cstdio>
using namespace zcps5::engine;
int main() {
  GamepadTracker tracker;
  Ps5PadState s{};
  s.connected=true;
  s.buttons=ps5bits::circle|ps5bits::cross|ps5bits::triangle|
      ps5bits::up|ps5bits::r1|ps5bits::options;
  s.ly=0;s.lx=255;s.r2=255;
  auto t=tracker.update(s);
  assert(t.current.connected&&t.current.action_a()&&t.current.action_b());
  assert(t.current.action_ex1()&&!t.current.action_ex2());
  assert(t.current.btn[14]&&t.current.btn[6]&&t.current.btn[8]);
  assert(t.current.btn[13]&&!t.current.btn[12]);
  assert(!t.current.btn[9]&&!t.current.btn[0]);
  assert(t.current.ly==-1.0f&&t.current.lx==1.0f);
  assert(t.down[1]&&t.down[2]&&t.down[4]&&t.down[13]);
  t=tracker.update(s);
  assert(!t.down[1]&&!t.up[1]);
  s.buttons=ps5bits::square|ps5bits::down|ps5bits::left|
      ps5bits::l1|ps5bits::touchpad|ps5bits::l2;
  s.lx=128;s.ly=128;s.r2=0;
  t=tracker.update(s);
  assert(t.current.action_ex2()&&!t.current.action_a());
  assert(t.current.btn[15]&&t.current.btn[16]&&t.current.btn[5]);
  assert(t.current.btn[7]&&t.current.btn[12]&&!t.current.btn[13]);
  assert(t.current.lx==0.0f&&t.current.ly==0.0f);
  assert(t.up[1]&&t.up[2]&&t.up[13]);
  s.connected=false;s.buttons=ps5bits::cross;s.lx=255;
  t=tracker.update(s);
  assert(!t.current.connected&&!t.current.action_b());
  assert(t.current.lx==0);
  assert(t.up[3]&&t.up[15]);
  s.connected=true;s.buttons=0;s.l2=128;s.r2=127;
  t=tracker.update(s);
  assert(t.current.btn[12]&&!t.current.btn[13]);
  puts("PASS: DualSense buttons map to original ZQuest PlayStation scheme indices");
}
