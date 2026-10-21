// SPDX-License-Identifier: GPL-3.0-or-later
// Links real ZQuestClassic ZASM command table to test the PS5 ABI.
// This is deliberately NOT a gameplay interpreter or a zplayer binary.
#include "components/zasm/table.h"
#include <cstdio>
#include <cstring>
#include <string>
extern "C" const char* zcps5_upstream_zasm_smoke() {
  static char result[200];
  const auto* frame = get_script_command(WAITFRAME);
  const auto* setv = get_script_command(std::string("SETV"));
  if (!frame || !setv || frame->command != WAITFRAME || setv->command != SETV)
    return "ZASM_TABLE_SMOKE=FAILED";
  std::snprintf(result, sizeof(result),
                "UPSTREAM_ZASM_TABLE_NATIVE_SMOKE WAITFRAME=%d SETV=%d",
                static_cast<int>(frame->command), static_cast<int>(setv->command));
  return result;
}
