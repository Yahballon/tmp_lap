# Godot 4.7.2 PS5 native GLES3 / Canvas2D — PPSA99847

The first **isolated** PS5 test intended to use Godot's actual
`DisplayServerPS5` and upstream `RasterizerGLES3` for drawing.

Unlike the stable v0.1.1 Scene Bridge (`PPSA99845`), this native wrapper
contains **no custom Sprite2D renderer**. It owns the proven PS5 EGL context,
exposes it to the Godot engine via `graphics_hooks_ps5.h`, and presents
only the frames Godot requests via `DisplayServerPS5::swap_buffers()`.
The initial flat blue boot clear is diagnostic and does **not** establish
that Godot produced a frame.

The actual original Godot `visual-smoke.pck` fixture contains `ColorRect`,
`Sprite2D`, RGBA8 checker texture and GDScript movement. X changes the
background if Input delivery survives the renderer bring-up.

Look for:
- `/data/GodotPS5/gles3-runtime.log`:
  `EGL_OPEN_OK`, `BOOT_FRAME_PRESENTED`, `GODOT_SWAP_FIRST`.
- `/data/GodotPS5/gles3-stderr.log`:
  `DISPLAY_SERVER_PS5_EGL_READY`, no GLAD initialization failures.
- `/data/GodotPS5/gles3-stdout.log`:
  `GODOT_PS5_REAL_2D_SCENE_READY` and controller markers.

**Not yet tested on PS5.** Build success is NOT proof of a working Canvas2D.
This test must not overwrite PPSA99843, PPSA99844 or PPSA99845.
GPL-3.0-or-later graphics glue and unofficial SDK provenance require
compliance review before any general distribution.
