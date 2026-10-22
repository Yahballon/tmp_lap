# Godot PS5 M8: external PCK + scene/asset acceptance

This is a **separate Godot 4.7.2 project** for testing the real
Godot GLES3/Canvas2D engine in `PPSA99847`, **without rebuilding the PS5
native executable**. It packages two Godot scenes, two GDScripts and
`res://palette.json` into `external-smoke.pck`.

## Build / download

Open [Build External Godot PS5 PCK](https://github.com/Yahballon/tmp_lap/actions/workflows/build-godot-ps5-external-pck.yml)
and download the **Godot-PS5-External-PCK-Host-Verified-NOT-PS5** artifact from
a green run. Green CI means only the desktop/headless Godot test passed.

Or use official Godot 4.7.2 on a PC:

```sh
godot --headless --path homebrew/godot-ps5/samples/external-pack-smoke \
      --script res://make_pack.gd
```

## Hardware test (PS5)

1. Install `PPSA99847 v0.1.2` from TESTBUILD; this is the existing native runner.
2. Copy `external-smoke.pck` to `/data/GodotPS5/games/external-smoke.pck`.
3. Create `/data/GodotPS5/launch.txt` containing exactly `external-smoke.pck`
   followed by a newline. Alternatively put the PCK in
   `/mnt/usb0/godot/external-smoke.pck` and write that *full absolute path*
   in `launch.txt`.
4. Launch PPSA99847. The first screen is a magenta ring on a deep-blue
   background. After 180 frames (about 3 s at 60 fps) it becomes a cyan
   circle around a dark square; after 240 more frames the app exits.
   Press **X** during either scene for a visible color change.
5. Inspect `/data/GodotPS5/gles3-runtime.log` for
   `PACK_SOURCE=CONFIG PACK=/data/GodotPS5/games/external-smoke.pck`,
   `GODOT_SWAP_FIRST`, `GODOT_RETURN=0`, `NORMAL_SHUTDOWN`,
   `EGL_CLOSED`.
6. Inspect `gles3-stdout.log` for
   `GODOT_PS5_EXTERNAL_MAIN_READY`,
   `GODOT_PS5_EXTERNAL_ASSET_OK`,
   `GODOT_PS5_EXTERNAL_SCENE_CHANGE_REQUESTED`,
   `GODOT_PS5_EXTERNAL_SECOND_SCENE_READY`,
   `GODOT_PS5_EXTERNAL_FINISHED`.
   `GODOT_PS5_EXTERNAL_GAMEPAD_X` or
   `GODOT_PS5_EXTERNAL_SECOND_SCENE_GAMEPAD_X` confirms the input test.
   Save `gles3-stderr.log` too.
7. **Remove launch.txt** to go back to the bundled Pong demo; the PCK
   may remain installed. An invalid file falls back to bundled Pong.

A failure to open the JSON file, load the second scene, or exit cleanly
is useful diagnostic evidence. Neither this fixture nor green CI proves
general third-party Godot compatibility, fonts, physics, audio or saving.
