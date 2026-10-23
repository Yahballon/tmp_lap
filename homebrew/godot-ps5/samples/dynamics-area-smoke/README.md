# M10a – Real RigidBody2D dynamics and Area2D triggers on PS5

**Purpose:** Extend successfully tested M9b native PhysicsServer2D with
*dynamic* body movement and monitoring signals. Standalone official Godot
4.7.2 PCK, **no new PS5 executable or custom collision simulation**.

The GDScript creates a `RigidBody2D` ball subject to real gravity, a
`StaticBody2D` collision platform, and an `Area2D` trigger. Godot's
real `area.body_entered`, `area.body_exited` and
`rigid_body.body_entered` signals must fire for PASS. The ball must
come to rest on the platform at the right height. A manually moved ball
or visual-only trigger *cannot* generate success markers.

## Installation on PS5

1. Use the **known-good PPSA99847 v0.1.3** native GodotPhysics2D
   build (M9b). Do not install the older v0.1.2 dummy-physics executable.
2. From the successful [M10a GitHub Actions workflow](https://github.com/Yahballon/tmp_lap/actions/workflows/build-godot-ps5-m10a-dynamics-area.yml),
   download the artifact (host-tested; **NOT** yet PS5-tested).
   Open the nested `GodotPS5-M10a-RIGIDBODY-AREA-TEST.zip`.
3. Copy the ZIP's `GodotPS5/` contents to `/data/GodotPS5/`.
   It provides `games/dynamics-area-smoke.pck` and replaces just
   `launch.txt`. Existing Pong and other external PCK files can stay.
4. Start `PPSA99847`. Expect a **yellow ball** to fall through the
   **cyan sensor**, cross three physics milestones, land on the
   **magenta floor** and rest. Three top lights become green; test
   exits after ~75 stable physics frames.

## Required log markers

```text
GODOT_PS5_M10A_BOOT
GODOT_PS5_M10A_BODIES_READY
GODOT_PS5_M10A_AREA_ENTER_OK
GODOT_PS5_M10A_AREA_EXIT_OK
GODOT_PS5_M10A_RIGID_CONTACT_OK
GODOT_PS5_M10A_TEST_PASS
```

Also collect `/data/GodotPS5/gles3-runtime.log` for
`PACK_SOURCE=CONFIG`, `GODOT_RETURN=0`, `NORMAL_SHUTDOWN`,
`EGL_CLOSED`. Capture `gles3-stderr.log` on failure.

**This only proves basic gravity/contact/trigger behavior**, not joints,
tilemaps, complex rigidbody interactions, performance, UI fonts, audio,
saves, or broad game compatibility.

Remove `/data/GodotPS5/launch.txt` to return to built-in Pong.
