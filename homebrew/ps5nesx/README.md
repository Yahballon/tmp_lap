# NESYX by YAHBALLON

NESYX is a native NES emulator and 3D cover-flow front end for jailbroken PS5 consoles. Powered by FCEUmm and the Project Porpoise-inspired front end.

**Version:** 1.0.26  
**Title ID:** PPSA99204  
**Release compatibility tag:** ps5nesx-v1.0.24 (kept for TESTBUILDS)

## Changes in v1.0.24

- New NESYX branding on the PS5 menu and home screen
- `by YAHBALLON | v1.0.24` under the application name
- Exact original high-resolution 1024 × 1536 NO COVER PNG embedded at `ps5/frontend/assets/no-cover.png`
- Game title shown alone, without the duplicate system subtitle
- Real covers still download and cache normally; tiger icon and existing settings are retained

**Compatibility:** Existing games and saves are still stored in `/data/PS5NES/` to avoid losing previous user data. The GitHub source folder and legacy release tag prefix remain `ps5nesx` for automated updates, but all displayed titles use NESYX.

This repository contains no game ROMs. Use only games you have the rights to use.

## Display sizing

New installs default to **16:9 Fullscreen** to fill the display without borders. The original 4:3 ratio and whole-integer pixel mode are available from Settings → Display → Picture. An existing saved NES/Aspect=4:3 or Integer setting takes precedence; choose 16:9 Fullscreen once to change it. Fullscreen intentionally stretches the classic NES image horizontally rather than cropping it. Changes are applied on game launch.
