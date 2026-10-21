# Godot 4.7.2 → PS5 Homebrew: engineering status

## Source tree
- Upstream [godotengine/godot](https://github.com/godotengine/godot), pinned to tag `4.7.2-stable`.
- PS5 target triple from PS5-DevBridge: `x86_64-sie-ps5`.
- `engine/platform/ps5/` is an external SCons platform overlay.
- `scripts/build-engine.sh` initiates a REAL experimental cross-compile of the Godot core, GDScript modules and the PS5 headless entry. No fake minimal game interpreter.
- `GODOT_PS5_NATIVE_IO` enables PS5 libkernel directory and PCK-header discovery, avoiding sandbox-incompatible libc scanning. This does not replace Godot's internal `FileAccessUnix` / `DirAccessUnix` yet.

## Acceptance gates
1. CI cross-compile Godot's actual engine source and pinned third-party modules with the public SDK; fix compiler errors as seen in CI logs.
2. Link Godot static archives into a native DevBridge `eboot.bin` via real CRT/import stubs; confirm the start/exit logs on jailbroken PS5.
3. Register PS5-native file/directory backends within Godot itself. Merely finding a PCK with kernel functions does **not** make Godot able to open its resources.
4. Run `samples/script-smoke` exported PCK and observe `GODOT_PS5_SMOKE_READY` on console.
5. Implement real Godot DisplayServer and renderer; next DualSense + audio driver; validate 2D scene, then 3D.
6. Only then distribute a homebrew title with an icon, README screenshot and live games launcher. Do not add TESTBUILDS early.

## Experimental manual build
```sh
git clone --depth=1 --branch 4.7.2-stable https://github.com/godotengine/godot.git /tmp/godot
GODOT_SOURCE=/tmp/godot \
PS5_PAYLOAD_SDK=/path/to/PS5-DevBridge/templates/simple-calc/.deps/native/ps5-payload-sdk \
BUILD_JOBS=4 bash homebrew/godot-ps5/scripts/build-engine.sh
```
This script may fail in SDK/toolchain API incompatibilities until fixed through the cross-compile CI job. Do not label a static archive a working executable or playable emulator.

## Current limitations
No PS5 native app is linked, no Title ID, no real renderer or audio, no console runtime/screenshot, and no public gameplay release. GDExtension, C#, 3D Vulkan and per-game compatibility not claimed.
