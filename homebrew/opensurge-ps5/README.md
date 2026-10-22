# Open Surge PS5 — return to original PS5 homebrew build system

**by YAHBALLON · v0.6.1 transparency + mask-collision test · PPSA99358**

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

**TESTBUILDS:** Refresh the catalog on your PS5 and select **Open Surge PS5 Test v0.6.1**. The release tag is `opensurge-ps5-v0.6.1` and the ZIP is `PPSA99358.zip`. This launches the native terrain preview and diagnostic, **not the original game engine**. The catalog must refresh after the release becomes available.


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


## v0.6.1 - Original demo entity subset

Based on the PS5-tested v0.2.4 native input and rendering stack.

Original levels/demo-1.lev includes 275 entity records. This version adds first native interactions for 124 original Collectibles (16fps sprite, once-only pickup and counter), one Spring Standard (600 world units/sec), two Spring Stronger (960 units/sec) and the original Goal sign.

Original images and sprites are loaded from the same pinned Open Surge upstream sources. X remains jump; Square resets the level and collectibles. Goal displays a completion flag, not a true stage transition.

SurgeScript execution, enemy AI, full damage physics, sound, full stages, checkpoints, and save states are still missing. This is an experimental native compatibility subset, not a complete Open Surge engine port.


## v0.6.1 - Original Surge movement and animation subset

Based on the player's successful firmware 5.10 test of v0.3.0; measured 59.5–59.6 FPS, original 124 collectible entities and spring mechanics confirmed.

Adds crouch with DOWN at rest, look-up with UP at rest, rolling by pressing DOWN while moving, and a Spin Dash: hold DOWN and press X to charge, tap X again for +0.25 (up to 1.0), release DOWN to launch at 480–720 world pixels/second. Uses upstream Open Surge `physicsactor.c` charge equations. Buffered DualSense held-state handling is unchanged from the verified v0.2.4 fix.

Maps genuine frames from pinned Open Surge `sprites/players/surge.spr`: idle 0; walking 1–8; running 9–16; jump and rolling 19–26; duck 39; look-up 36; charge 20/22/24/26; brake 29–32; falling 42–45. This is a targeted native animation state mapping, not SurgeScript playback.

Fixes spring rapid retrigger from the uploaded v0.3.0 console log at x≈2928: a spring must be left and its cooldown elapsed before firing again.

Tests native C++ spins/charges/rolls and the real upstream data; all old collision and PS5 input smoke tests remain. Enemies, sound, authentic sensor physics, all level transitions and save games remain incomplete.

## v0.6.1 — Original enemies, hazards and damage (native compatibility subset)

After console confirmation of stable 60fps movement, animated rings, springs and the Spin Dash, this native release adds the original demo's **5 Mosquitos, 1 Wolfey, 1 Lady Bugsy, 6 Spikes** with their original PNG sprites and level coordinates. Approximate enemy behavior is derived from the pinned 0.6.1.3 SurgeScripts: Mosquitos fly left at 60 px/s, Wolfey patrols at 80, Lady Bugsy walks at 30 and fires simple 60 px/s projectiles, and spikes hurt on contact. Rolling or landing on an enemy defeats it and awards 100 points. Player hits trigger 1.6 seconds of invulnerability; a hit with collectibles loses them (scattered rings not yet implemented); otherwise the player loses one of three lives and respawns, with game over at zero. Square restarts all lives and enemy states; Circle returns to the main menu. PS5 verified pad-state logic remains unchanged.

**Not full original Open Surge:** native enemy physics and bullets are simplified; full SurgeScript gameplay VM, authentic obstacle-aware enemy walking/AI, dynamic layers/tubes/bridge interactions, sound/music, item effects, damage-ring scattering, original stage transitions and savegames remain unavailable. Upstream attribution and original assets preserved.

## v0.6.1 – Original Open Surge sound effects and level music

Adds **16 authentic Open Surge WAV SFX** (jump, collectible, ring loss, spring, Spin Dash charge/recharge/release, roll, brake, damage, death, enemy defeat, spikes, enemy shot, goal and checkpoint) and the exact **`musics/template.ogg`** named in the original `levels/demo-1.lev`. ffmpeg only converts the pinned upstream files to PS5 UI's required 48 kHz stereo PCM SFX / Ogg Vorbis. The existing `hui::audio::Mixer` with `hui::ps5::AudioOut` streams decoded music into sceAudioOut from a separate thread and plays the original effects through 32 software voices. The buffer is refilled from the main thread about 0.5s ahead; no external service is required. Triangle toggles music when playing; music is silent on the main menu. Optional audio initialization never blocks the already verified graphics or DualSense controls; console runtime checks report `ORIGINAL_AUDIO_INIT`, `AUDIO_PERF`, underruns and audio output errors in `/data/OpenSurge/legacy-port.log`.

**Still incomplete:** the original SurgeScript gameplay VM, full enemy AI/collision/terrain interactivity, dynamic rings-on-hit, all stage transitions and saves. This revision needs an on-console audio quality and stability test.

## v0.6.1 — uphill Spin Dash collision fix

Real PS5 screenshots show an uphill stall at player world position **x=2500, y=10015, VX=0 and 60 FPS**. The native movement class mistakenly used upright collision probes even during a high-speed roll, causing the front corner to misinterpret a true textured ramp as a solid vertical wall. Original `physicsactor.c` defines a smaller 15x29 rolling collision sensor: v0.6.1 switches to 14px-wide, 25px-tall native rolling probes and preserves authentic mask-based floor movement. Rolling momentum is now influenced by sampled true terrain slope, with the original upstream uphill and downhill forces (281.25 / 1125 units/s²) and the existing rolling friction. The host CI test warps to the user's actual screenshot coordinates in test-only code, charges Spin Dash and requires the player to advance up the authentic ramp and gain height. Stable buffered DualSense, OpenGL terrain, native original SFX/music and combat stay untouched. Console retest required.

## v0.6.1 – Stop Spin Dash passing through solid terrain

User's actual 6.7-second PS5 video of v0.5.1 shows the character partially tunneling through the big Sunshine ramp and falling beneath solid ground, although rendering remains at 59.6 FPS with clean audio. Root cause: old `roll_uphill_step` wrote the next X position while moving Y upward by only 2–4px; its target was verified, but the entire path through intermediate solid pixels was not. The replacement **never advances X into the obstacle**. For a fast rolling approach to a real reachable upper mask surface, Surge stays on the wall/contact X coordinate, moves upwards continuously at bounded speed, and only resumes horizontal motion when the next rolling body position is collision-free. The contact state blocks normal falling physics during the vertical traversal and exits on the real upper ledge. Normal jumping, collectibles, hazards, sound, music, 120-Hz stepping and pad buffering unchanged. CI additionally tests for body-in-solid and under-floor physics failures (not just maximum X). The result remains an approximation of original wall/loop physics; real PS5 validation required.

## v0.6.1 — Original world layers and basic Tube In/Out scripts

The pinned original `demo-1.lev` contains **11 collision-layer-specific bricks** (6 green, 5 yellow) and Layer Green / Layer Yellow activation entities, none of which the old native preview recognized. The 0.5.3 asset builder now preserves their layer labels, and the C++ collision mask selects default/green/yellow according to the original `obstaclemap.c` filter. All 6 original `Tube In` and 6 `Tube Out` locations are staged. A limited native scripted route transports a rolling player between compatible authored entry/out points at 600 world px/s with input locked and a boosted exit, matching core numbers in `scripts/items/tubes.ss`. Unsupported complex curves and loop physics **remain unimplemented**. The original engine has 4 sensor orientations (floor/right wall/ceiling/left wall); the preview does not. This build **does not claim that loopings or all tubes work**. All existing graphics/music/Spin Dash/combat preserved; host CI specifically tests green/yellow transitions and two-way original tube endpoints.


## v0.6.1: Safe tube exit guard

The real PS5 v0.5.3 video shows the player leaving an underground tube at about x=4496 and falling below the map because the native four-mode wall/ceiling loop physics isn't linked. This revision prevents tube routes from launching the character into a destination without clear body geometry, a real mask-supported floor, and safe forward foot contact. In an unsupported x=3500–4860 loop corridor, a fall more than 130px below the last confirmed grounded position is recovered to that position. This is a controlled safety fallback, not full loop support. The original audio, controllers, collectibles and enemies are unchanged. Logs include TUBE_UNSAFE_EXIT and LOOP_GUARD_RESTORE counters.

## v0.6.1 — four original Sunshine stage QA bundle

Bakes pinned upstream **demo-1, sunshine-1, sunshine-2 and sunshine-3** authored maps into individual native stage caches, preserving their original tile positions and collision masks. Use L1/R1 (or D-pad left/right) in the menu to pick any stage and X to start. Original Goal/Goal Capsule touch advances to the next stage; Checkpoint touch changes the native respawn point if collision-safe. Selected stage, completion unlock and checkpoint are saved atomically in `/data/OpenSurge/progress.cfg`. For testing, all stages remain manually selectable before unlock. Previous native sound effects, music, gameplay subset and input remain.

**Still not the complete Open Surge engine:** SurgeScript/Allegro are not integrated, level-specific soundtrack switching and many objects (bridges, powerups, ziplines, angled springs, loop physics, bosses, original ending) are unsupported or approximated. Sunshine-3 capsule is only a trigger, not original boss fight. This is a broad QA build, not a finished upstream port.

## v0.6.1 — original stage-specific music with crossfade

The stage pack already contains authentic pinned upstream `music` metadata, but v0.6.0 played `musics/template.ogg` on every level. v0.6.1 stages two *real* tracks, `template.ogg` (demo-1) and `sunshine.ogg` (Sunshine 1/2/3), converted by ffmpeg to 48-kHz stereo Vorbis in the asset cache. PS5 GameAudio uses two independent decoding ring buffers/mixer stream slots, keeping both alive while `sceAudioOut` is running. Stage selection / auto-advance fades between the correct source streams (0.18 seconds), without resetting the PS5 audio device or losing game SFX. Triangle toggles music as before, and the diagnostic log prints `STAGE_MUSIC <level> <track> ORIGINAL_TRACK_OK` on stage load. The original gameplay/physics, savegame, checkpoint and four stage layouts remain unchanged. CI verifies the exact original per-level music mapping and both valid Vorbis streams. Console audio validation is required. The upstream SurgeScript/Allegro gameplay engine remains unported.
