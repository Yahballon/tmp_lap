// SPDX-License-Identifier: GPL-3.0-or-later
// Host smoke: no JIT code executed here. Only memory rights are tested.
#include "jit_probe.hpp"
#include <cassert>
#include <cstdio>
int main(){
  const rxbench::JitProbe p=rxbench::probe_wx_transition();
  std::printf("mapped=%d rx=%d rw_restored=%d unmapped=%d errno=%d stage=%s\n",
              p.mapped,p.rx,p.rw_restored,p.unmapped,p.error,p.failed_at);
  assert(p.passed()); // supported on GitHub Linux CI; console separately measured
  std::puts("PASS: RW -> RX -> RW transition without RWX or execution");
  return 0;
}
