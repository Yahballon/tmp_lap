# Native PS5 port engineering

This is an **actual original-engine integration effort**, not a native imitation of the game. A playable PS5 app is **not yet available**.

## What is proven in source

- Exact original Open Surge v0.6.1.3 sources (revision lock), including gameplay, scripts, engine scenes, levels, graphics and audio asset manifests.
- Original SurgeScript v0.6.1 and PhysicsFS v3.2.0 source revisions.
- An isolated `OPENSURGE_PS5` launch integration which points the engine's `--game-folder` to `/data/homebrew/PPSA99358/game` and requests persistent preferences under `/data/OpenSurge`.
- Source-drift guards, unit tests and Linux original-engine build CI. This tests host compilation, **not gameplay on PS5**.

## Cross-platform implementation barriers

1. **Allegro 5.2.11.3 for PS5.** Upstream requires the main library and 11 addons: image, primitives, color, font, TTF, audio, audio codecs, native-dialog, memfile, physfs and main. No verified PS5-native Allegro package has been found. Allegro's experimental `ALLEGRO_SDL=ON` backend is a promising starting point, but depends on complete SDL2 gamepad/window/OpenGL/audio functionality in the target runtime.
2. **SDL2 / OpenGL compatibility.** Validate the PS5 OpenGL SDL bridge's available symbols and behavior; provide missing SDL2 APIs and a controller mapping if necessary. It must support GL context creation, swap/present, textures and audio latency without relying on Linux drivers.
3. **SurgeScript and PhysicsFS.** The next CI job now attempts to cross-compile the original full SurgeScript VM and PhysicsFS into PS5 static libraries through PS5-DevBridge's compiler wrapper; compilation status must be checked in GitHub Actions. This is not a linkable or playable game until runtime thread, storage, ZIP and archive behavior is verified on hardware.
4. **Game data and filesystem.** Bundle all redistributed upstream art, scripted levels, music, fonts, licenses and language files under `PPSA99358/game/`. Validate PhysicsFS mounts from that PS5 location and persistent saves. Do not write into a read-only mount.
5. **Native title packaging.** Link/sign a real PS5 `eboot.bin` using the verified PS5-DevBridge runtime; include `sce_sys/param.json`, original licensed app icon and required `sce_module` runtime. Run PS5 installer validation. A generic Linux ELF is not `eboot.bin`.
6. **Console acceptance.** On firmware 5.10 test boot, intro, title screen, a complete level, controller, music/SFX, saving/reloading, game exit and at least one user-made level or MOD.

## Acceptance: when this may be labeled playable

Only once all six requirements work on an actual PS5 and the installer was inspected may CI publish `PPSA99358.zip` and the TESTBUILDS catalog list it. Neither this branch nor its Linux build claims success there.

## Source attribution

Open Surge Engine by Alexandre Martins and contributors, GPL-3.0-or-later: https://github.com/alemart/opensurge. SurgeScript copyright its authors; Allegro 5 and PhysicsFS retain their independent license terms. Include all upstream license files with an eventual release.
