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
