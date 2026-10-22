# Godot PS5 Pong — Real GLES3 Canvas2D Gameplay Test

An experimental **Godot 4.7.2 Godot/GDScript Pong mini-game** intended
as the default bundled PCK of PPSA99847 v0.1.2. It uses the **actual
Godot `Node2D._draw()` commands** (rectangles, lines, circles, alpha),
Godot `Input` and GDScript game logic. No standalone OpenGL quad renderer,
no Font/FreeType dependency, and no unverified Godot PhysicsServer2D.

Controls:

- **Left stick up/down or D-pad up/down:** player paddle.
- **X:** serve ball after start or each score.
- **Circle:** reset match.
- **Options:** quit cleanly.
- Bot controls the other paddle. The score is rendered with custom
  seven-segment vector digits rather than unsupported native fonts.
- Automatic shutdown after 180 seconds for hardware validation.

Expected PS5 logs in `/data/GodotPS5/gles3-stdout.log`:
`GODOT_PS5_PONG_READY`, `GODOT_PS5_PONG_SERVE`,
`GODOT_PS5_PONG_PADDLE_INPUT`, `GODOT_PS5_PONG_60_FRAMES`,
`GODOT_PS5_PONG_HIT`, `GODOT_PS5_PONG_SCORE`,
`GODOT_PS5_PONG_OPTIONS_EXIT`, or
`GODOT_PS5_PONG_TIMELIMIT_EXIT`.
`gles3-runtime.log` must show `GODOT_SWAP_FIRST` and
`NORMAL_SHUTDOWN`.

Godot's GLES3 Canvas2D has already been demonstrated by another
checkerboard scene, but **this Pong fixture has not been hardware tested**.
Passing cross-compilation alone does not confirm playable framerate or
input response. Known-good v0.1.1 release remains available.

Technical caveats: hand-coded paddle/ball collisions only;
no claim that native Godot physics, fonts, 3D, audio, or
exported third-party games are fully compatible.
