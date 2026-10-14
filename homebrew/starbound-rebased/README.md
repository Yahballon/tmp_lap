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
