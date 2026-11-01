// SPDX-License-Identifier: GPL-3.0-or-later
// Reusable, host-testable state for PS5 RandomX automatic benchmark suites.
#pragma once
#include <cstdint>

namespace rxbench {
struct AutoResult {
  int profile=-1;
  int outcome=0;
  uint64_t hashes=0;
  uint64_t elapsed_us=0;
  double hps=0.0;
};
struct AutoSuite {
  bool active=false, done=false, cancelled=false, with_full=false;
  int step=0, recorded=0, total=0, duration=30;
  AutoResult results[5]{};

  void begin(bool include_full, int seconds) {
    active=true; done=false; cancelled=false; with_full=include_full;
    step=0; recorded=0; total=include_full?5:4; duration=seconds;
    for(auto& result:results)result=AutoResult{};
  }
  void cancel() { if(active)cancelled=true; }
  // Called exactly once after the current benchmark worker was joined.
  // Returns true if the next profile should begin.
  bool record(int outcome, bool completed, uint64_t hashes, uint64_t us) {
    if(!active || recorded!=step || step>=total)return false;
    AutoResult& r=results[step];
    r.profile=step; r.outcome=outcome; r.hashes=hashes; r.elapsed_us=us;
    r.hps=us?double(hashes)*1000000.0/double(us):0.0;
    ++recorded;
    if(completed && !cancelled && recorded<total) { ++step; return true; }
    active=false;done=true;
    return false;
  }
};
} // namespace rxbench
