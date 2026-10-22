# Godot 4.7.2 PS5 Homebrew Runtime

**by YAHBALLON** · Title ID `PPSA99843` · original Godot Engine (MIT)

## Proven on real PS5

The **v0.2.11 headless build actually executes Godot GDScript on the console**.
A console TAR recorded:

```text
Godot Engine v4.7.2.stable.custom_build
GODOT_PS5_SMOKE_READY
```

The native `engine-smoke.log` independently recorded
`NATIVE_GODOT_RETURN=0` and a PS5 shell-exit request. This is a meaningful
engine milestone: we are running the real native Godot VM, resource loader,
project settings, main loop, and a GDScript scene, not emulating Godot.

**Not yet a playable game port:** Godot still runs headless; there is no PS5
DisplayServer, Vulkan or OpenGL renderer, audio driver, or DualSense-to-Godot
Input bridge. The built-in `smoke.pck` exits after printing the verification
marker. PS5 console behavior of each new release must be tested separately.

## PS5 hardware discovery and v0.3.2 fix

User's 22 Oct 2026 PS5 console log (TAR #13) confirms native
`[GodotPS5 PAD] CONNECTED dev=0` and `[GodotPS5 PAD] CROSS_DOWN`.
However its selected pack is `PACK_SOURCE=AUTOSCAN PACK=/mnt/usb0/godot/smoke.pck`.
That USB fixture is older and prints only `GODOT_PS5_SMOKE_READY`:
it has no `GODOT_PS5_DUALSENSE_TEST_WAITING` marker. Thus the native button
is demonstrated, but its forwarding to *GDScript* is not yet proven.

v0.3.2 fixes the test collision: automatic discovery ignores a file named
`smoke.pck` (case insensitive) in external folders, but still launches real
external games such as `Demo.pck`. The bundled current smoke pack then
runs when no other game PCK is detected. A user may explicitly select an
external `smoke.pck` with `/data/GodotPS5/launch.txt` if desired.
Run the bundled smoke and check for `GODOT_PS5_DUALSENSE_TEST_WAITING`,
`GODOT_PS5_DUALSENSE_CONNECTED`, and
`GODOT_PS5_DUALSENSE_CROSS_PRESSED` before claiming success.

## Next native milestone: DualSense → Godot Input (v0.3.2 test)

The **experimental** controller bridge now uses the native PS5 `scePad` and
user-service interfaces to pass controller input to the real Godot
`Input` singleton. Cross/Circle/Square/Triangle, D-pad, L1/R1, L3/R3,
Options, touchpad click, dual analog sticks and analog triggers are mapped.
Native samples are consumed in batches to preserve short presses.
Connection, disconnection, short-lived controller interception and retry
are handled without using the app's UI-specific pad classes.

Host mapping and 120-byte sample ABI tests are present. **This is not yet
proven on PS5 hardware**. The built-in `smoke.pck` still immediately prints
`GODOT_PS5_SMOKE_READY`, then waits up to **20 seconds** for a controller:
press X (Cross) on the DualSense and read these console logs:

```text
/data/GodotPS5/engine-stderr.log   # [GodotPS5 PAD] CONNECTED / CROSS_DOWN
/data/GodotPS5/engine-stdout.log   # GODOT_PS5_DUALSENSE_CONNECTED
                                   # GODOT_PS5_DUALSENSE_CROSS_PRESSED
```

A headless test has **no display output**; the logs are the acceptance
criteria, not a visible controller UI. If an external PCK is explicitly selected by `launch.txt`, or a non-smoke
external game is auto-detected, it runs instead of the bundled controller test.
To test the bundled fixture, remove `launch.txt` and temporarily move all
external `.pck` files other than old `smoke.pck` out of the scanned folders.

The **graphics** milestone is next: an isolated GPU/display probe and then a
real Godot DisplayServer / renderer. DualSense events do not by themselves
enable visual gameplay.

## Next milestone: universal PCK launcher (v0.3.0 test)

The experimental launcher now supports **standalone Godot 4 .pck projects**
without rebuilding the executable for each project.

Launch order:

1. `/data/GodotPS5/launch.txt` — a text file containing one absolute PCK
   path or a basename such as `Demo.pck` (which selects
   `/data/GodotPS5/games/Demo.pck`)
2. First alphabetically valid `.pck` in
   `/data/GodotPS5/games/`, `/mnt/usb0/godot/`,
   `/mnt/usb1/godot/`, `/mnt/usb2/godot/`,
   `/mnt/usb3/godot/` or `/mnt/usb0/GodotPS5/games/`
3. The packaged `/app0/assets/godot/smoke.pck` fallback

Example `launch.txt`:

```text
/mnt/usb0/godot/MyGodotGame.pck
```

The kernel-based selector checks native PCK headers and restricts external
paths to dedicated game folders. The next CI build tests these rules using
host and simulated PS5-kernel tests; **external-game launching has not yet
been verified on hardware.** Projects requiring a graphical renderer, 2D/3D
physics, sound or normal gamepad input are not compatible yet.

Runtime logs:

```text
/data/GodotPS5/engine-smoke.log
/data/GodotPS5/engine-stdout.log
/data/GodotPS5/engine-stderr.log
```

Look for `PACK_SOURCE=CONFIG`, `PACK_SOURCE=AUTOSCAN` or
`PACK_SOURCE=BUNDLED_SMOKE` to confirm which PCK was selected. Look for
`GODOT_PS5_SMOKE_READY` **only** when the bundled smoke script runs.

Read [GRAPHICS_BRINGUP.md](GRAPHICS_BRINGUP.md) for the concrete DisplayServer, Vulkan/OpenGL, DualSense, audio and hardware-verification roadmap.

## Engineering roadmap

1. **Real Godot VM and PCK I/O:** proven for the built-in v0.2.11 smoke test;
   v0.3.0 adds external project selection and tests.
2. **DualSense Input:** implement and verify PS5 pad connection, polling,
   button/axis events and Godot `Input` integration, without changing SDL
   or a non-PS5 platform.
3. **DisplayServer and rendering:** implement PS5 video output and a genuine
   Godot-compatible graphics path; evaluate Vulkan/Godot RenderingDevice
   compatibility with public PS5 homebrew SDK before claiming game support.
4. **Audio + timing + savegames:** native sound and reliable per-game
   persistence, lifecycle, crash recovery and gameplay testing.
5. **General Godot games:** test one real Godot 4 2D project end-to-end,
   then 3D, shader and engine-feature compatibility.

## Build and source

Sources live under `engine/platform/ps5` and are overlaid on pinned upstream
Godot **4.7.2-stable**. `scripts/build-engine.sh` cross-compiles genuine Godot
static archives with Clang 18; native PS5-DevBridge links a signed
`PPSA99843/eboot.bin`, including the test PCK and normal
`sce_sys` package metadata. GitHub Actions validates host test fixtures and
provides signed installers through trusted default-branch publishing.

`ENGINE_PORTING.md` documents platform internals, native linker details,
source patches and runtime acceptance criteria.

## Licensing

- Original Godot Engine — [godotengine/godot](https://github.com/godotengine/godot) (MIT)
- PS5 homebrew port — YAHBALLON (MIT, see [LICENSE](LICENSE))
- Public PS5 SDK, PS5-DevBridge and other dependencies retain their own licenses
