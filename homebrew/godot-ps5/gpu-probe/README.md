# Godot PS5 – isolated GPU bring-up probe

**Experimental, not Godot gameplay.** This is a completely separate test
title **PPSA99844** (initial build v0.1.0); the proven headless Godot
GDScript runtime stays PPSA99843 and is never changed by this probe.

## Acceptance conditions on PS5 hardware

After installing `PPSA99844.zip`, a real PS5 screen should display a sequence
of **blue → green → red fullscreen frames**, switching automatically about
every two seconds. DualSense **X** moves to the next color, and **Circle**
exits cleanly. The app also closes after at most 1800 frames.

The test writes `/data/GodotPS5/graphics-probe.log`. The critical markers
are `EGL_OPEN_OK`, `FIRST_FRAME_PRESENTED`, `PRESENTED_FRAME_COUNT=60`
and `CLEAN_SHUTDOWN`. Capture a real console screenshot and this log; CI
cross-link and package validation alone do not prove display works on PS5.

## Implementation & licenses

Uses the existing `Yahballon/PS5-DevBridge` native
`templates/simple-calc` as a **GPL-3.0-or-later** base, with its
`ps5-homebrew-ui` EGL/GL adapter and pinned `ps5-opengl` SDK. This
probe itself is GPL-3.0-or-later; third-party code stays under its own
licenses. Its source, native OpenGL dependencies, corresponding source and
notices must be reviewed before any broad public release.

- GPU app source: `main.cpp`
- CI: `.github/workflows/build-godot-ps5-gpu-probe.yml`
- Output: `PPSA99844.zip`, attached only as a GitHub Actions test artifact
- **No catalog entry** until signed build and real firmware 5.10 validation
- This GPU test is not compiled into the MIT-licensed Godot engine

Next milestone after hardware success: implement real `DisplayServerPS5`
and connect to the upstream Godot 4.7.2 OpenGL compatibility renderer.
