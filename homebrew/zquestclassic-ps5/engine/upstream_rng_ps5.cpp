// SPDX-License-Identifier: GPL-3.0-or-later
// A genuine upstream engine-base function is linked into the PS5 bringup
// artifact. It is not gameplay and must not be presented as such.
#include "base/random.h"
#include <cstdio>
extern "C" const char* zcps5_upstream_engine_smoke() {
  static char report[120];
  zc_randgen local;
  zc_srand(42, &local);
  const auto n = static_cast<unsigned>(zc_rand(&local));
  std::snprintf(report, sizeof(report),
                "UPSTREAM_RNG_NATIVE_SMOKE seed=42 result=%u", n);
  return report;
}
