# DualSense → ZQuestClassic PlayStation gamepad controls (engine milestone)

**Status: native PS5 bridge compiled, not yet an Allegro joystick driver. No gameplay.**

The user confirmed on PS5 that v0.1.5 draws the indexed 8-bit GPU
test pattern and loads both quest files. The next actual game-engine blocker
is producing gamepad inputs in the format the original ZQuestClassic
`zplayer` expects.

## Upstream contract

Original ZQuestClassic at upstream pinned revision
`c44dde7d0146da78bf8b541af457d5ff4e67a830`:
`src/zc/control_scheme.cpp::make_gamepad_default_scheme()` maps
1-indexed SDL-style Allegro-Legacy buttons. PlayStation pads are assigned
the **Nintendo/SNES arrangement**, independent of the PlayStation symbols:

| Original ZQuest action | DualSense position | 1-indexed joystick button |
| --- | --- | --- |
| A | Circle (east) | 2 |
| B | Cross (south) | 1 |
| Ex1 (X) | Triangle (north) | 4 |
| Ex2 (Y) | Square (west) | 3 |
| Up/down/left/right | D-pad | 14/15/16/17 |
| L/R | L1/R1 | 5/6 |
| Start | Options | 8 |
| Select | Touchpad click | 7 |
| Ex3/Ex4 | L2/R2 | 12/13 |

Stick axes are converted from byte samples (neutral 128) to -1..+1.
L2/R2 accept both binary button flags and analog intensity >= 128.
The Guide/PS key is not exposed by PS5-DevBridge's existing pad sample and
is correctly left inactive. No shortcut touches system-reserved buttons.

## Files

- `dualsense_zplayer.hpp`: pure C++ mapping, disconnect handling, state
  transitions (pressed/released/held) for 1-indexed legacy gamepad buttons.
- `dualsense_zplayer_test.cpp`: host unit tests for all principal mappings,
  analog triggers, sticks, disconnection and button edges.
- `ps5_pad_adapter.hpp`: consumes the **actual**
  `hui::PadSample` from PS5-DevBridge, with no custom kernel-pad driver.
- `ps5_pad_adapter_probe.cpp`: links in isolated PS5 engine build and
  produces `PS5_ZPLAYER_INPUT_MAPPING_OK` if a synthetic native sample
  maps to the original layout.

The engine bringup GitHub Action compiles the real adapter with the PS5 SDK
and validates its presence in the FSELF, without updating the public
TESTBUILDS release.

## Still blocking actual zplayer gameplay

1. Implement the **ALLEGRO_JOYSTICK** input-driver and event-queue bindings
   to expose these buttons/sticks to upstream `control_scheme` and menus.
   This adapter alone does not send `ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN`.
2. Wire the 8-bit PS5 framebuffer bridge to **actual** Allegro-Legacy
   `BITMAP` pages produced by `zplayer`, not the proof/test pattern.
3. Compile and run complete upstream `zplayer`, script VM, assets,
   Allegro sound/timer drivers, quests, save/load and display.
4. Verify real gameplay on console before offering a playable ZIP.

All changes keep GPLv3 attribution of ZQuestClassic and PS5-DevBridge.
