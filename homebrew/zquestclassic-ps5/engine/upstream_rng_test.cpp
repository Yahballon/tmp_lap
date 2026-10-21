// SPDX-License-Identifier: GPL-3.0-or-later
// Linked with the ORIGINAL ZQuestClassic/src/base/random.cpp at the pinned
// upstream revision. No substitute RNG is used.
#include "base/random.h"
#include <cassert>
#include <cstdio>
int main() {
  zc_randgen left, right;
  zc_srand(0x345678, &left);
  zc_srand(0x345678, &right);
  for (int i = 0; i < 1000; ++i)
    assert(zc_rand(&left) == zc_rand(&right));
  for (int i = 0; i < 1000; ++i) {
    const auto n = zc_rand(7, -7, &left);
    assert(n >= -7 && n <= 7);
  }
  zc_srand(42, &left);
  const auto first = zc_rand(&left);
  zc_srand(42, &right);
  assert(first == zc_rand(&right));
  puts("PASS: actual upstream ZQuest core RNG linked and deterministic");
}
