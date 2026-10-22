# PS5 Godot Scene Bridge: PPSA99845 (EXPERIMENTAL)

This test combines **the real Godot 4.7.2 GDScript engine** and
**hardware-proven PS5 OpenGL/EGL** in a single PS5 process. A real Godot
`Sprite2D` node updated by GDScript controls the position and rotation of
an OpenGL checkerboard drawn by a *separate native preview renderer*.
A real Godot `count` script property changes the background on DualSense X.

**Not yet Godot's own RenderingServer/CanvasItem/GLES3 renderer.**
The next milestone is migrating the working EGL context into
`DisplayServerPS5` and connecting the actual GLES3 compatibility backend.
This is an important engine-to-GPU integration test, NOT a complete port.

- Title: PPSA99845 — never replaces PPSA99843 or PPSA99844.
- Test app: blue-grey background + moving/rotating pink/cyan translucent
  checkerboard. DualSense **X** changes background red and blue, counted
  by GDScript itself.
- App quits automatically after the real GDScript scene runs 20 seconds.
- Persistent logs: `/data/GodotPS5/scene-bridge.log`,
  `bridge-stdout.log`, `bridge-stderr.log`.
- Expected: `GODOT_PS5_REAL_2D_SCENE_READY`,
  `GODOT_GDSCRIPT_SCENE_FRAME_DRAWN`, `FIRST_DRAW_GL_ERROR=0`,
  `FIRST_FRAME_PRESENTED`, `GDSCRIPT_CROSS_COUNT=1`,
  `GODOT_PS5_REAL_2D_CROSS_PRESSED`, `NORMAL_SHUTDOWN`.
- The PS5 GL shim from PS5-DevBridge uses GPL-3.0-or-later, and the
  checksum-pinned unofficial OpenGL SDK mirror needs license/provenance
  review. Original Godot source is MIT. Never claim this composite binary
  is MIT-only; retain corresponding build sources and notices.
- Install to TESTBUILD **only after** PS5 native signed ZIP and GitHub
  publisher are verified. No false release for failing CI.

Hardware test needs console screenshot **and** logs. The GDScript
input counts and graphics must be observed together for acceptance.

## Firmware 5.10 black-screen fix — v0.1.1 (22 Oct 2026)

The v0.1.0 console TAR `bfpilot-download (15).tar` confirms
`EGL_OPEN_OK`, `SCENE_BRIDGE_GL_READY`, `GODOT_ENGINE_ENTER`, but
**no frame presentation** and no GDScript scene-ready marker. The
`bridge-stderr.log` contained dozens of
`FreeType: Can't load dynamic font, engine is compiled without FreeType
support!` errors, traced to `Label.new()` in `res://main.gd:34`.
This created an error storm before `_ready()` could complete.

v0.1.1 removes the font-requiring `Label` and its updates, without
changing Sprite2D, ColorRect, Input, or GDScript animation. It **first
presents a blue boot framebuffer** (`BOOT_FRAME_PRESENTED`) before
loading Godot, so a pre-scene hang is visible and clearly distinguishable
from a GPU problem. A second log marker `GODOT_GDSCRIPT_SCENE_FRAME_DRAWN`
is emitted only when a real Godot scene frame is submitted. The original
v0.1.0 prerelease remains immutable.

Acceptance: see the blue start background and then moving pink/cyan
checkerboard controlled by the Godot scene. X should cycle background
colors. Submit fresh `scene-bridge.log`, `bridge-stdout.log`,
`bridge-stderr.log`. This remains an independent OpenGL preview,
**not** Godot's GLES3 renderer.

## v0.1.1 hardware accepted — 22 October 2026

User reports a visible pink/cyan checkerboard and DualSense X changing
the background color. In uploaded `bfpilot-download (16).tar`, the third
(0.1.1) run in `scene-bridge.log` contains:

```text
BOOT_FRAME_GL_ERROR=0
BOOT_FRAME_PRESENTED
GODOT_ENGINE_ENTER
GDSCRIPT_CROSS_COUNT=1
GODOT_GDSCRIPT_SCENE_FRAME_DRAWN
FIRST_DRAW_GL_ERROR=0
FIRST_FRAME_PRESENTED
GDSCRIPT_CROSS_COUNT=24
PRESENTED_FRAME_COUNT=600
```

`bridge-stdout.log` prints `GODOT_PS5_REAL_2D_SCENE_READY` followed
by **24** `GODOT_PS5_REAL_2D_CROSS_PRESSED` lines. No FreeType errors
occur in that latest run, although older failed runs are appended in
`bridge-stderr.log`. The newest run was captured **before automatic exit**
so `NORMAL_SHUTDOWN` cannot yet be claimed. Two recoverable warnings
remain (optional project.binary and global script cache, plus dummy 2D
physics). The scene is a real Godot scene, but the GL bridge remains
a separate preview rasterizer, *not* Godot's own RendererGLES3.
