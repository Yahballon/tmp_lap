// SPDX-License-Identifier: GPL-3.0-or-later
// Host-testable sequential benchmark scheduler. The actual CPU work runs
// outside this state machine, and the next profile starts only after join.
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
  enum Kind : int { Light4=0, LightFull5=1, SecureJit3=2 };
  bool active=false, done=false, cancelled=false, with_full=false;
  Kind kind=Light4;
  int step=0, recorded=0, total=0, duration=30;
  int plan[5]{0,1,2,3,4};
  AutoResult results[5]{};

  void begin(bool include_full, int seconds) {
    begin_kind(include_full?LightFull5:Light4,seconds);
  }
  void begin_jit(int seconds) { begin_kind(SecureJit3,seconds); }
  void begin_kind(Kind k,int seconds) {
    active=true;done=false;cancelled=false;
    kind=k;with_full=(k==LightFull5);
    step=0;recorded=0;duration=seconds;
    const int light[5]={0,1,2,3,4};
    const int jit[3]={7,10,11}; // 1/2/4 workers; none use full-memory mode
    total=(k==SecureJit3)?3:with_full?5:4;
    for(int i=0;i<5;i++) {
      plan[i]=k==SecureJit3 && i<3?jit[i]:light[i];
      results[i]=AutoResult{};
    }
  }
  int current_profile() const { return step<total?plan[step]:-1; }
  void cancel() { if(active)cancelled=true; }
  // Called only after worker join, at most once per stage.
  bool record(int outcome, bool completed, uint64_t hashes, uint64_t us) {
    if(!active || recorded!=step || step>=total)return false;
    AutoResult& r=results[step];
    r.profile=plan[step];r.outcome=outcome;r.hashes=hashes;r.elapsed_us=us;
    r.hps=us?double(hashes)*1000000.0/double(us):0.0;
    ++recorded;
    if(completed&&!cancelled&&recorded<total){++step;return true;}
    active=false;done=true;
    return false;
  }
};
} // namespace rxbench
