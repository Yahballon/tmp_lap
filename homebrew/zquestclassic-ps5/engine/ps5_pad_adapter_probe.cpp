// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/ps5_pad_adapter.hpp"
// Statically link native sample -> command mapping to verify SDK ABI.
// No injection into upstream ZQuest controls until zplayer is linked.
extern "C" const char* zcps5_pad_adapter_smoke() {
  hui::PadSample s{};
  s.connected=true;
  s.buttons=hui::pad_bits::kCircle|hui::pad_bits::kTriangle;
  const auto mapped=zcps5::engine::map_ps5_pad(
      zcps5::engine::from_hui_sample(s));
  return mapped.action_a()&&mapped.action_ex1()&&!mapped.action_b() ?
         "PS5_ZPLAYER_INPUT_MAPPING_OK" : "PS5_ZPLAYER_INPUT_MAPPING_FAILED";
}
