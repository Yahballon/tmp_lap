# Starbound Rebased — Gold872 engine port (work in progress)

This is a **clean, separate porting track**, not the procedural Starbound Sprint engine.

## Exact upstream
- Repository: https://github.com/Gold872/Super-Mario-Bros
- Revision: `609c5af217abf6eb55d3eb594df4db76cfb0c90c`
- C++ code license: MIT, Copyright (c) 2022 Gold87; preserve upstream LICENSE.

## Scope
Retain the actual C++ engine's ECS, scenes, commands, physics, collision, camera, animation and Tiled-CSV loading. Do **not** ship its copyrighted Nintendo-themed sprites, tiles, logos, font assets, audio, text or levels. They are **not** covered safely merely because the source code is MIT.

The first CI milestone compiles the **real unmodified upstream C++ code** against SDL2 on Linux without distributing upstream resources. It is **not** a PS5 build. A PS5 port needs a functioning SDL2 video/input/audio implementation for the PS5 (or a compatibility layer); the existing EGL/OpenGL scaffold is not an SDL2-compatible runtime.

Asset replacement requires **exact source-frame rectangle compatibility**: 25x16 player sprite cells, 35x15 enemy cells, 48x22 tile cells at 16 source pixels each, as used by the engine's Map ID tables. All output assets must be original, and new maps must be authored, not shuffled Nintendo originals.

## Safe release gate
A playable installable PS5 ZIP must not be published until 1) platform backend works, 2) independently created animation sheets are integrated, 3) all 32 levels plus transitions are rewritten, 4) music/SFX and visible UI are replaced, and 5) real PS5 tests pass. Keep existing PPSA99641 v0.7.0 untouched for rollback until then.

## CI
`Build Gold872 Engine Port (source smoke test)` validates that the precise upstream engine can compile. It does not download Nintendo assets or publish any APK/PKG/ZIP.

## v0.1.6 resource layout

The native engine resolves `res/...` paths from its runtime working directory. With `chdir("/app0/assets")`, the installable ZIP must contain `PPSA99642/assets/res/{sprites,fonts,data,sounds}/...`. This layout is asserted during CI and the PS5 app reports `ASSET00`–`ASSET04` in its boot log. Do not flatten the `res` directory into `assets`.


## Optional HD artwork loader (v0.1.13)

The PS5 binary can pick up the user's separately supplied new pink/cyan art pack from `/mnt/usb0/homebrew/starbound-rebased-hd/res/sprites/` or `/data/starbound-rebased/hd/res/sprites/`. It requires all five core PNGs (PlayerSpriteSheet, EnemySpriteSheet, BlockTileSheet, logo, alien_sky) before enabling, so partial installs safely fall back to built-in graphics. **The new HD PNG files are not bundled in the GitHub code-only build**. For a standalone PS5 ZIP with replacement images, patch an existing tested `PPSA99642.zip` with the separately delivered art pack's `make_hd_release.py`.

## v0.1.14 — built-in original HD pixel artwork (no external pack required)

The six **actual artist-supplied PNG files** (pink astronaut player, five enemy families, magenta/teal blocks, pixel logo, app icon, and illustrated blue sky) are committed under `homebrew/starbound-rebased/hd-assets/{characters,blocks,backgrounds,icons}/`. The build runs `install-hd-art.py` immediately after the reference asset generator, then puts the source-identical images into `PPSA99642/assets/res/sprites/`. CI compares the packaged PNGs byte-for-byte with these committed files before publishing `starbound-rebased-v0.1.14`. No second download or USB override is necessary.

PS5 engine still uses the existing 25×16 / 35×15 / 48×22 fixed atlas addressing and reference World1-1 mechanics. Actual animation and gameplay behavior must be tested on hardware.

## v0.1.15 — correct HD atlas placement and visual hierarchy

Fix for the real player spritesheet coordinate issue visible on hardware: the engine samples each 16-pixel cell starting at Y=9 in the source sheet (Y=18 in doubled HD). v0.1.14 incorrectly placed each row at Y=2, cutting the player animation frames. This version stores frames at exact hard-coded SDL source coordinates, repairs 1px enemy padding, composites the tall armored enemies as a continuous 2-tile sprite, and uses the correct slime family for World1-1 Goomba IDs 70/71. It also simplifies the sky (strong dark blue contrast under the HUD) and makes the repeated ground tile a proper solid pink rock rather than an isolated grassy top in every vertical layer. Supersedes outdated external HD artwork overrides; embedded artwork always wins. All PS5 ZIP PNGs are matched byte-for-byte against the tracked files. PS5 gameplay still needs verification by the developer after installation.

## v0.1.16 native-size sprite atlas compatibility

Restore the exact Gold872 reference atlas dimensions (player 403x266, enemies 562x242, blocks 817x375) and **remove 2x source-rectangle scaling** in C++. A new reference-guided palette and silhouette reskin makes the sprites pink/cyan while preserving every original transparency boundary and sprite frame. This avoids accidental humanoid fireballs, two small bodies in a power-up, pipes drawn instead of boxes, and cloud cells as rectangles. CI enforces all original alpha bytes and dimensions for each packaged sprite atlas, with added rollback-safe installation in TESTBUILDS. This is a geometry-safe cosmetic reskin, not yet a painstaking custom AI redraw of each frame. Test on a PS5 before considering this production-stable.

## v0.1.17 — original user AI characters in exact 1x sprite slots

Native AI astronaut sprite frames are added to small (16x16) and big power-up (ONE 16x32) player source rectangles, instead of recoloring old character silhouettes. Fireball IDs 246 and 247 render a cyan projectile/explosion rather than mini-astronauts. Original enemy ID 70/71 uses supplied pink slime artwork, crawler 38/39 uses its one-frame 16x32 model. Exact native canvas sizes, physics, HUD and tile ID mapping stay unchanged; block sky/pipe cloud masks are still matched pixel-for-pixel against pinned source.
