// SPDX-License-Identifier: GPL-3.0-or-later
// Runtime adapter from the actual PS5-DevBridge PadSample layout.
// This produces data for a future upstream ALLEGRO_JOYSTICK shim.
#pragma once
#include "engine/dualsense_zplayer.hpp"
#include "core/input.hpp"
namespace zcps5::engine {
inline Ps5PadState from_hui_sample(const hui::PadSample& raw) {
  Ps5PadState s{};
  s.connected=raw.connected;
  s.buttons=raw.buttons;
  s.lx=raw.left_x;s.ly=raw.left_y;
  s.rx=raw.right_x;s.ry=raw.right_y;
  s.l2=raw.l2;s.r2=raw.r2;
  return s;
}
}
