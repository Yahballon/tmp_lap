# Starbound Sprint v0.6.0

Original cosmic platform adventure for the homebrew-enabled PlayStation 5. Eight extended worlds, 16-bit-inspired pixel art extracted from the **original generated Starbound Sprint concept atlas** supplied in the project, with no commercial Nintendo assets.

**Title ID:** PPSA99641 — **Content Version:** 01.000.006 — **Author:** YAHBALLON

## Gameplay

- Eight extended stages with four full sprite-art biomes: Overworld, Crystal Cave, Magma Fortress and Cloud Ruins (including advanced remixes).
- 64 atlas frames: 8 player animation poses, 8 enemy types, 8 pickups, 32 environment tiles and 8 props, plus four panoramic concept backgrounds.
- Enhanced original tile map with staggered platforms, pits, springs, spike traps, collectable paths and moving platforms.
- Camera-follow, layered parallax scenes, animated collectibles, hazards, creature movement and HUD.
- Classic acceleration and gravity, double-jump (Cross twice), stamina-free sprint, enemy stomps, score and checkpoints.
- Existing progress format and Title ID are unchanged: `/data/homebrew/PPSA99641/starbound-progress.cfg`.

## DualSense Controls

| Control | Action |
| --- | --- |
| D-Pad / Left Stick | Move |
| Cross | Jump / second airborne jump; confirm on menus |
| Square | Sprint |
| Options | Pause / resume |
| Circle | Return to title; exit from title screen |
| Triangle | Continue unlocked progress from title screen |

## Art pipeline

`assets/sprite-atlas.b85` stores 64 cropped and re-quantized sprites derived from the project's originally generated concept image. `prepare_assets.py` decodes the indexed atlas once **at build time** to `app/assets/starbound-sprites.rgba` (128×128 pixels, RGBA8). Four concept backdrops are independently decoded to \`starbound-backgrounds.rgba\` and GPU-uploaded at initialization. No costly per-frame PNG decoding takes place on PS5; the atlas is uploaded once and rendered as batched texture quads with nearest-neighbor filtering. The art is a cohesive first playable conversion of the approved concept; original individual sprite source drawings may be expanded in later updates.

## Build

[Build Starbound Sprint](../../.github/workflows/build-starbound-sprint.yml) compiles with PS5-DevBridge and ps5-homebrew-ui, runs host physics/level tests, verifies `eboot.bin`, native libraries and the actual compiled asset data, then creates `PPSA99641.zip`.

## Installation

Get [the v0.6.0 release](https://github.com/Yahballon/tmp_lap/releases/tag/starbound-sprint-v0.6.0), extract the `PPSA99641` folder into `/data/homebrew/` and register with a compatible homebrew launcher. **Back up `starbound-progress.cfg` before replacing the app folder.**

## Known limitations

No music / sound effects yet; frame timing currently targets 60 Hz. Tests validate the PS5 build and independent gameplay logic, not operation on physical PS5 hardware.

## License

GPL-3.0-or-later, preserving upstream framework attribution.

## Fantasy visual remaster (v0.6.0)

Removes heavily magnified 16-pixel terrain and character art from gameplay. Renders original native-resolution 1080p shapes instead: moonlit scene, multi-layer parallax clouds, floating islands and castles, light shafts, animated waterfalls, lit lanterns, grass-covered rocky cliffs, crystal particles and a larger white-haired explorer. All platforms and enemies still follow real physics/collision objects; saved progress and controls are unchanged. This is a code-driven playable interpretation of the reference illustration, **not an identical reproduction of the single flattened image**. The old low-res atlases remain in the package temporarily for compatibility, but they are no longer rendered during gameplay. Real PS5 visual testing and higher-resolution authored art remain future work.

## Sound system (v0.6.0)

Native PS5 `libSceAudioOut` (48 kHz stereo signed 16-bit, 256 frames per output block) with a dedicated mixer thread. Original synthesized chiptune music varies across four biomes; gameplay sound effects include first jump, double jump, crystals, enemy stomp, hurt, spring, menu, pause, stage clear, death, win and checkpoint. Triangle during gameplay toggles mute. Audio gracefully disables itself if the native module is unavailable; real hardware audio behavior needs PS5 verification.

## Tiled-style native PS5 level architecture (v0.6.0)

Porting the entire [Gold872/Super-Mario-Bros](https://github.com/Gold872/Super-Mario-Bros) desktop SDL2 runtime unchanged would not produce a PS5 binary. This project instead ports the **data-driven Tiled CSV layer/entity architecture** to its existing native PS5 C++ runtime and retains the existing AudioOut, DualSense, save data and shader rendering. Eight completely new CSV-authored fantasy stages have unique platforms, crystal placement, holes, enemies and spring/hazard routes. Original code is independently implemented; Nintendo source textures, level layouts and music are never copied. Gold872's codebase is MIT licensed; its proprietary-looking bundled assets should not be assumed freely reusable.
