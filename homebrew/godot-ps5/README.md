# Godot PS5 Engine Port

**Godot 4.7.2 native engine bring-up · by YAHBALLON**

**Experimental source only – NOT PLAYABLE yet.**

This project adds a PS5 homebrew SCons platform overlay to the original [Godot Engine](https://github.com/godotengine/godot), not a mock games browser or an emulator.

## Current code
- `engine/platform/ps5/` custom OS layer, static-library compiler config, and Godot `Main::setup`/run entry point.
- Native PS5 `sceKernelOpen`, `sceKernelRead`, `sceKernelGetdents` PCK detection; host fallback and tests.
- Validates Godot 4 standalone PCK magic, pack-format version and compatible engine major/minor; rejects symlinks on host and via O_NOFOLLOW on PS5.
- CI host unit tests and **real upstream Godot experimental PS5 cross-compile job**.
- GDScript `samples/script-smoke` fixture and real upstream Godot Linux PCK generation/execution in CI.

## Future game folders
```text
/data/GodotPS5/games/your-game.pck
/mnt/usb0/godot/your-game.pck
/mnt/usb1/godot/your-game.pck
```
These folders are a planned runtime interface, not a claim that an installable PS5 launcher exists.

## Validation
```sh
python3 homebrew/godot-ps5/scripts/check_overlay.py
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror homebrew/godot-ps5/tests/pck_locator_test.cpp -o /tmp/godot-ps5-test
/tmp/godot-ps5-test
```

Read [ENGINE_PORTING.md](ENGINE_PORTING.md) for cross-build instructions, console linker work and mandatory engine/backend milestones.

The CI `godot-4-gdscript-smoke-pck-NOT-PS5` artifact is a real Godot 4 test project pack, **not** an installable PS5 game. The Linux run must log `GODOT_PS5_SMOKE_READY` to pass.

## Artwork / screenshots
No game screenshot or PS5 app icon exists yet. `engine/platform/ps5/logo.svg` is a small original vector placeholder for the Godot source tree, not an AI icon or installed console art.

## Credits & licensing
- [Godot Engine](https://github.com/godotengine/godot) upstream contributors — MIT.
- YAHBALLON — experimental PS5 homebrew port files, MIT, [LICENSE](LICENSE).
- Public PS5 homebrew SDK/PS5-DevBridge are separately licensed.
