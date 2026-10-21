# Open Surge PS5 — return to original PS5 homebrew build system

**by YAHBALLON · v0.3.0 transparency + mask-collision test · PPSA99358**

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

**TESTBUILDS:** Refresh the catalog on your PS5 and select **Open Surge PS5 Test v0.3.0**. The release tag is `opensurge-ps5-v0.3.0` and the ZIP is `PPSA99358.zip`. This launches the native terrain preview and diagnostic, **not the original game engine**. The catalog must refresh after the release becomes available.


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

## v0.2.3 – movement and performance

The user's real PS5 video confirms correct original artwork, but movement was choppy. This revision builds a spatial index for original collision masks, advances movement in fixed 120Hz physics steps and waits only the remainder of a 16.67ms frame instead of imposing another full 16ms delay after GPU present. Acceleration, friction and jump are improved within the native preview. The HUD reports measured FPS and horizontal velocity; /data/OpenSurge/legacy-port.log records PERF diagnostics roughly every five seconds. CI compiles and executes the exact native level class against original Open Surge data in a host smoke test. This remains a test preview, not complete SurgeScript/Open Surge gameplay.

## v0.2.3 — right-only slope-collision repair

Recorded PS5 0.2.2 gameplay at **59.6 FPS** showed that movement stops at world x=723/900 even when only RIGHT is held; FPS was not the bottleneck. The old horizontal collision used the character's body probes against terrain BEFORE following the ground mask, repeatedly classifying sloping floors as solid walls (VX=0), toggling grounded/landed. This revision uses original per-pixel mask top-surface transitions across three foot sensors. When walking, it follows slopes by up to 12 world pixels per substep and steps down up to 22; real walls still block. Falling and jumping still use the original mask. A genuine bottomless drop auto-respawns instead of infinite sky camera scroll. Native diagnostics add TERRAIN_BLOCK and RESPawn counts. CI now runs the real C++ controller for 900 frames with RIGHT continuously held and asserts it crosses the visible slope into the map area (>x=1000), without asking the user to repeatedly reset.

The renderer, texture coordinates and complete original game data are unchanged. This remains a preview physics approximation; original game engine/SurgeScript isn't linked.

## v0.2.4 — PS5 controller buffered-input fix (based on real console logs)

Hardware v0.2.3 logs showed **59.6 FPS with no terrain block** through x=1346 but repeated LEVEL_JUMP even when RIGHT was the only intended input. Root cause in native/main.cpp: `pad.read()` returns a count of **new** samples, often zero; old code reset `held=0` when no sample was provided, falsely releasing continuous RIGHT and recreating CROSS edges for still-held buttons. New `native/pad_state.hpp` persists the final controller state on empty polls, aggregates genuine edges across buffered samples, only releases on explicit disconnected/intercepted samples, and uses input priority between D-pad and left stick. The original level, slope physics and rendering are unchanged. `native_pad_state_smoke.cpp` replays 600 frames with only 1 packet every 4 frames, requiring 600/600 continuous right and zero phantom jump events. New `PAD_DIAG` logs report actual raw/held mask and empty-poll statistics.

If the test still jumps without CROSS, capture `PAD_CROSS_EDGE raw=...` messages to identify hardware-reported input. This remains a native level preview and does not run original SurgeScript gameplay.


## v0.3.0 - Original demo entity subset

Based on the PS5-tested v0.2.4 native input and rendering stack.

Original levels/demo-1.lev includes 275 entity records. This version adds first native interactions for 124 original Collectibles (16fps sprite, once-only pickup and counter), one Spring Standard (600 world units/sec), two Spring Stronger (960 units/sec) and the original Goal sign.

Original images and sprites are loaded from the same pinned Open Surge upstream sources. X remains jump; Square resets the level and collectibles. Goal displays a completion flag, not a true stage transition.

SurgeScript execution, enemy AI, full damage physics, sound, full stages, checkpoints, and save states are still missing. This is an experimental native compatibility subset, not a complete Open Surge engine port.
