# M9a – Godot 4.7.2 real 2D physics / CharacterBody2D on PS5

The **separate external PCK** tests whether the native Godot engine in
already installed **PPSA99847 v0.1.2** implements **REAL** Godot PhysicsServer2D.

The main Godot scene constructs CharacterBody2D, StaticBody2D, and
CollisionShape2D/RectangleShape2D. The character moves right with
\`move_and_collide()\`; real collision with the wall and the expected
surface normal are required for \`GODOT_PS5_M9_REAL_COLLISION_OK\`.
Purely visual changes and scripted Pong-style bounce do NOT count.

## PS5 hardware test

After a successful [M9 host CI](https://github.com/Yahballon/tmp_lap/actions/workflows/build-godot-ps5-m9-physics-pck.yml),
download the artifact containing a PS5 setup ZIP.

1. Keep installed \`PPSA99847 v0.1.2\`. No new eboot.bin is used.
2. Copy the ZIP's \`GodotPS5/\` contents to \`/data/GodotPS5/\`:
   \`games/physics-smoke.pck\` and \`launch.txt\`. This replaces the
   **previous M8c launch.txt** intentionally; the M8c PCK remains on disk.
3. Start the existing Godot PS5 GLES3 app.
4. The cyan square should move into the pink wall and **stop**.
   It changes green only when **real collision API** confirms wall contact;
   a green circle appears at the top. Press X optionally to log input.
5. After 300 physics ticks of the green result, the app exits.
6. Inspect \`/data/GodotPS5/gles3-stdout.log\` for
   \`GODOT_PS5_M9_BODIES_READY\`,
   \`GODOT_PS5_M9_REAL_COLLISION_OK\`, and
   \`GODOT_PS5_M9_TEST_PASS\`.
   Inspect \`gles3-runtime.log\` for \`PACK_SOURCE=CONFIG\`,
   \`GODOT_RETURN=0\`, \`NORMAL_SHUTDOWN\`, and \`EGL_CLOSED\`.
   Include \`gles3-stderr.log\` for failures.

**If the square passes through the wall or logs
\`GODOT_PS5_M9_NO_COLLISION_PHYSICS_UNAVAILABLE\`, that is a useful failure:
do not mark M9 passed!** It means the native engine needs the real
PhysicsServer2D enabled, which requires separate native engine work.

Remove \`/data/GodotPS5/launch.txt\` to restore built-in Pong, or write
\`external-smoke.pck\` in launch.txt to return to the verified M8c demo.

A green host CI run establishes that this **PCK has genuine Godot collision
logic** on desktop Godot 4.7.2. It does **not** prove PS5 physics support.
