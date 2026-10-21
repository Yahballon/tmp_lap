// SPDX-License-Identifier: GPL-3.0-or-later
// DualSense -> ZQuestClassic Allegro-Legacy gamepad numbering adapter.
// Source of numbering and PlayStation action bindings:
// ZQuestClassic/src/zc/control_scheme.cpp make_gamepad_default_scheme().
// Pure data adapter. DOES NOT implement ALLEGRO_JOYSTICK driver, event loop,
// or make zplayer playable; future shim consumes these numbered buttons.
#pragma once
#include <array>
#include <cstdint>
namespace zcps5::engine {
namespace ps5bits {
constexpr uint32_t options=0x00000008u, up=0x10u, right=0x20u;
constexpr uint32_t down=0x40u, left=0x80u, l2=0x100u, r2=0x200u;
constexpr uint32_t l1=0x400u, r1=0x800u, triangle=0x1000u;
constexpr uint32_t circle=0x2000u, cross=0x4000u, square=0x8000u;
constexpr uint32_t l3=2u, r3=4u, touchpad=0x00100000u;
}
struct Ps5PadState {
  uint32_t buttons=0;
  uint8_t lx=128,ly=128,rx=128,ry=128,l2=0,r2=0;
  bool connected=false;
};
struct ZplayerPadState {
  // Buttons numbered 1..17; slot 0 is deliberately always unused.
  std::array<bool,18> btn{};
  float lx=0,ly=0,rx=0,ry=0;
  bool connected=false;
  // Upstream PlayStation face default (SNES arrangement):
  // A=east=2, B=south=1, Ex1=north=4, Ex2=west=3.
  bool action_a() const {return btn[2];}
  bool action_b() const {return btn[1];}
  bool action_ex1() const {return btn[4];}
  bool action_ex2() const {return btn[3];}
};
inline float axis(uint8_t v) {
  // Keep raw +/- range, center is exactly 0.0 at DualSense neutral 128.
  const int n=int(v)-128;
  return n<0 ? float(n)/128.0f : float(n)/127.0f;
}
inline ZplayerPadState map_ps5_pad(const Ps5PadState& s) {
  ZplayerPadState z{};
  if (!s.connected) return z;
  z.connected=true;
  z.lx=axis(s.lx);z.ly=axis(s.ly);z.rx=axis(s.rx);z.ry=axis(s.ry);
  const auto test=[&](uint32_t b){return (s.buttons&b)!=0;};
  // SDL standard positional gamepad model (1-indexed in ZQuest controls).
  z.btn[1]=test(ps5bits::cross);z.btn[2]=test(ps5bits::circle);
  z.btn[3]=test(ps5bits::square);z.btn[4]=test(ps5bits::triangle);
  z.btn[5]=test(ps5bits::l1);z.btn[6]=test(ps5bits::r1);
  z.btn[7]=test(ps5bits::touchpad);z.btn[8]=test(ps5bits::options);
  // Button 9 = Guide, not exposed by PS5-DevBridge's PadSample.
  z.btn[9]=false;
  z.btn[10]=test(ps5bits::l3);z.btn[11]=test(ps5bits::r3);
  z.btn[12]=test(ps5bits::l2)||s.l2>=128;
  z.btn[13]=test(ps5bits::r2)||s.r2>=128;
  z.btn[14]=test(ps5bits::up);z.btn[15]=test(ps5bits::down);
  z.btn[16]=test(ps5bits::left);z.btn[17]=test(ps5bits::right);
  return z;
}
// Source-level state transitions for future ALLEGRO_JOYSTICK button events.
struct PadTransition {
  ZplayerPadState current{};
  std::array<bool,18> down{},up{};
};
class GamepadTracker {
 public:
  PadTransition update(const Ps5PadState& sample) {
    PadTransition transition{};
    transition.current=map_ps5_pad(sample);
    for (int n=1;n<=17;++n) {
      transition.down[n]=transition.current.btn[n]&&!last_.btn[n];
      transition.up[n]=!transition.current.btn[n]&&last_.btn[n];
    }
    last_=transition.current;
    return transition;
  }
 private:
  ZplayerPadState last_{};
};
} // namespace zcps5::engine
