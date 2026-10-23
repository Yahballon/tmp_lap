# M12 – Real Third-Party 2D Platformer #2201 on PS5 (Godot 4.7.2)

**Upstream original game:** [2D Platformer – Starter Kit 1.1.1](https://godotengine.org/asset-library/asset/2201), by **G2P Studios / AdilDevStuff**, original [GitHub repository](https://github.com/AdilDevStuff/2D-Platformer-Starter-Kit), pinned to commit `a48fc994cbcb168be472df84f2e13b55d2a2792f`, **MIT license**. Original 2D sprite assets credited to Kenney.nl, sound FX credited to Gdfxr. The original MIT license is preserved **inside the test installer ZIP and PCK**.

## Why this is a milestone

This is **not** a new trivial Godot smoke scene. It imports and runs the
**actual original third-party 2D platform game**, including:

- 2 original authored levels, original player animations and Camera2D
- CharacterBody2D floor collision, gravity, jumping, optional double-jump code
- Multiple real TileMapLayer ground and trap collision layers
- Animated sprites, CPUParticles2D, coin pickup and HUD/score
- Scene changes through LevelFinishDoor, original manager autoloads
- Original TrueType `Assets/Fonts/Star Choco.ttf` and four real `.wav` effects

No original gameplay code is rewritten. Only a **PS5Controls** autoload
maps existing `Left`, `Right`, `Jump` input actions to the DualSense
and logs scene loading and player position. Compatibility renderer replaces
upstream Forward Plus because our native Godot PS5 v0.1.6 is GLES3-based.
One desktop-only tutorial text is changed to the PS5 button names.

## Reproducible build

[GitHub Actions #39930320591](https://github.com/Yahballon/tmp_lap/actions/runs/39930320591)
**PASSED** on the original Godot 4.7.2 headless application.

- GitHub checkout of exact upstream asset pinned commit
- Editor import of all original PNG tilesets, fonts, WAV sound effects
- **92** packed original/game/imported files, genuine Godot 4.7 PCK
- Independent engine boot with `--main-pack`, Level_01 loaded
- PS5 DualSense actions created, CharacterBody2D player at (150,318)
- Host player fell onto actual ground and was grounded at
  `(150,447.959)` by physics tick 180
- No Godot script/scene loading errors detected in host acceptance
- Installable ZIP passed CRC, retained full upstream MIT license,
  PCK magic `GDPC`, format 4, Godot version 4.7
- PCK SHA256:
  `df286a34ac6df63950a7b73cd394dc5ee9975c1251dac53bcab940eab3a240ac`

**Important: this proves the third-party project loads and plays in desktop
Godot 4.7.2 headless, NOT yet on actual PS5 hardware.** v0.1.6 remains
the proven console engine version. It may still reveal missing image
texture importer support, other native modules or audio file decoding on
PS5, which must be diagnosed from console logs.

## PS5 testing – v0.1.6, PPSA99847

Download `GodotPS5-M12-Platformer-Asset2201-PS5-TEST.zip` from the
successful GitHub Actions artifact.

1. Extract it and copy its **`GodotPS5`** folder contents into the
   existing `/data/GodotPS5/` folder (merge files).
2. It installs only
   `/data/GodotPS5/games/m12-platformer-asset2201.pck`, and sets
   `/data/GodotPS5/launch.txt` to
   `m12-platformer-asset2201.pck` (with a newline).
3. Open your already installed **Godot PS5 v0.1.6** title
   `PPSA99847`. **No native `eboot.bin` change** and no update
   or new title ID is required.
4. Use **left stick or D-Pad** to move and **Cross (X)** to jump.
5. Test both levels, jumping on platforms, coins/score,
   camera scrolling, traps/respawn, scene transition and
   audible jumping/pickup/completion sound effects.
6. Send one photo/video of actual rendered level and a TAR with
   `/data/GodotPS5/gles3-stdout.log`,
   `gles3-stderr.log`, `gles3-runtime.log`.
   Host-only `SCENE_READY` markers do not prove console graphics.

Expected PS5 log markers from the unmodified original game
plus compatibility adapter:
```text
GODOT_PS5_M12_INPUT_READY DualSense left-stick DPad CROSS
GODOT_PS5_M12_SCENE_READY Level_01
GODOT_PS5_M12_PLAYER_FOUND position=(150.0, 318.0)
GODOT_PS5_M12_FRAME tick=180 ...
GODOT_PS5_M12_CROSS_JUMP
GODOT_PS5_M12_SCENE_READY Level_02
```

To return to the default Godot Pong sample, delete only
`/data/GodotPS5/launch.txt`. Older game PCKs, persistent saves and
the native `PPSA99847 v0.1.6` installation remain untouched.

## Next action after PS5 hardware test

**Until the player confirms this third-party platformer renders and plays
correctly on an actual console, M12 is 🟡 HOST-VALIDATED, HARDWARE-UNTESTED.**
If PS5 fails to boot/render/load textures, use real `gles3` logs to isolate
the missing native resource decoder/module. Don't invent green hardware
status from desktop CI.

Sources: MIT original upstream asset pinned at the SHA above, plus
`PS5Controls.gd`, `prepare_project.py`, `pack_m12.gd`,
`package_platformer.py` (PS5 wrapper code) in this folder.
