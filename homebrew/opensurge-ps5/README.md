# Open Surge PS5 — return to original PS5 homebrew build system

**by YAHBALLON · v0.2.2 transparency + mask-collision test · PPSA99358**

**Status: first native rendering/movement preview of genuine demo level; the original Open Surge engine is NOT integrated. This revision fixes screenshot magenta-key transparency, adds original-image mask collisions, single-sample controller debouncing and on-screen position/grounding diagnostics.** The experimental Allegro 5 cross-build was not compatible with the desired workflow. The active PS5 test pipeline now uses the **same PS5-DevBridge + ps5-homebrew-ui + native OpenGL** app packaging architecture used by 4 WINS and other working native PS5 homebrew.

- [Native legacy PS5 diagnostic workflow](../../.github/workflows/build-opensurge-legacy-ps5.yml) — builds a correctly signed/installable-format PS5 homebrew test app, not the upstream game.
- [Original Open Surge engine host verification](../../.github/workflows/build-opensurge-ps5.yml) — source compatibility check; experimental native C library job retained for research, not the default console packaging path.

## What the native PS5 test actually does

1. Boots a genuine native PS5 OpenGL + DualSense application with the old DevBridge system.
2. Stages **all 682 original Open Surge game files** (count observed in CI for v0.6.1.3; authoritative file count is calculated from SHA-256 manifest during each build), including quests, level data, music, fonts, scripts, sprites and license files.
3. Displays the genuine game's image and tests opening and reading representative engine asset files **from the PS5 app mount**.
4. Logs results to **/data/OpenSurge/legacy-port.log** so actual console behavior can be verified.
5. **Cross** enters the new real level renderer; left stick/D-pad moves the original Surge sprite; **Cross** jumps; **Square** resets; **Circle** returns. This now samples the original `sunshine_mask.png` collision pixels, implements gravity/jump and a first bounded slope step. It remains a physics prototype, NOT the original engine.
6. The original diagnostics remain reachable, and the whole game asset archive stays in the release.

It does **not** run SurgeScript gameplay scripts, the original Allegro game loop, enemies, rings or correct native level physics. The rendering and movement probe is a stepping stone towards those systems, not a complete port. Native app boot is only an infrastructure milestone; it is not a full game port.

## How to test the native build

**TESTBUILDS:** Refresh the catalog on your PS5 and select **Open Surge PS5 Test v0.2.2**. The release tag is `opensurge-ps5-v0.2.2` and the ZIP is `PPSA99358.zip`. This launches the native terrain preview and diagnostic, **not the original game engine**. The catalog must refresh after the release becomes available.


Open GitHub Actions > **Build Open Surge PS5 Legacy Native** > successful run > **PPSA99358 Open Surge LEGACY PORT DIAGNOSTIC**, download the artifact and extract its **PPSA99358.zip**. The containing folder has the usual **eboot.bin**, **sce_sys/** and **sce_module/** layout. This native test artifact is published as an explicitly labeled GitHub **prerelease**, discoverable by TESTBUILDS via version.json. It is **not playable Open Surge**.

Place the PPSA99358 folder under /data/homebrew/ using a compatible homebrew loader, then inspect **/data/OpenSurge/legacy-port.log** for **ASSET_READ_OK** markers. This is an experiment; firmware 5.10 compatibility and console rendering remain to be verified.

## Engineering paths

| Directory / file | Description |
|---|---|
| native/main.cpp | Native PS5 interface, screen transitions, pad controls and diagnostic log |
| native/level_preview.hpp | Original tile/sprite render with first movement/collision test (not original gameplay engine) |
| scripts/prepare-level-preview.py | Convert original demo-1.lev and sunshine.brk image sheet into a native render cache |
| scripts/test-level-preview.py | Ensure all brick definitions and original textures are staged accurately |
| scripts/prepare-legacy-assets.py | Bundle the complete original game files and preview asset |
| scripts/validate-legacy-package.py | Reject malformed native packages |
| scripts/stage-game-data.py | Verifiably stage all licensed original Open Surge assets |
| upstream.lock.json | Pin upstream GPL engine and dependencies |
| ps5/ | Experimental earlier Allegro engine cross-compile research, not the active native test |

## Goal

A **full port** must eventually reproduce all game logic, physics, characters, original assets, sound, menu and save behavior through a proven PS5 renderer and input/audio implementation. The current native diagnostic is a reproducible first step, not evidence this goal is achieved.

Original game by Alexandre Martins and the Open Surge contributors under GPLv3. Keep their full license metadata and attribution with any distributed game resources. See [PORTING.md](PORTING.md).

## v0.2.2 – movement and performance

The user's real PS5 video confirms correct original artwork, but movement was choppy. This revision builds a spatial index for original collision masks, advances movement in fixed 120Hz physics steps and waits only the remainder of a 16.67ms frame instead of imposing another full 16ms delay after GPU present. Acceleration, friction and jump are improved within the native preview. The HUD reports measured FPS and horizontal velocity; /data/OpenSurge/legacy-port.log records PERF diagnostics roughly every five seconds. CI compiles and executes the exact native level class against original Open Surge data in a host smoke test. This remains a test preview, not complete SurgeScript/Open Surge gameplay.
