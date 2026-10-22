# Godot 4.7.2 → PS5 EGL/GLES3 integration contract

**Status: planned engineering steps, not a working Godot renderer.**

Two independent console-tested paths now exist:

- `PPSA99843`: actual native Godot 4.7.2 GDScript engine + PS5 controller.
- `PPSA99844`: non-Godot EGL/OpenGL 4.6 accelerated rendering and
  textured alpha-blended geometry.

The goal is to integrate these *without* silently swapping Godot
GDScript output for a stand-alone renderer.

## 1. Platform build and renderer dependencies

Godot platform overlay `engine/platform/ps5/detect.py` currently advertises
`opengl3=False`, and `scripts/build-engine.sh` forces `opengl3=no`.
Both intentionally produce a headless smoke library. Build a separate
**experimental** engine configuration with `opengl3=yes`, enabling the
upstream `drivers/gles3` and shader/compiler resources required by the
Compatibility rendering method. Do not change stable headless defaults.

Upstream Godot 4.7.2 reference code:

- `servers/display/display_server.h` — registration of create function,
  main window/display contracts, `process_events`, `swap_buffers`.
- `platform/linuxbsd/x11/display_server_x11.cpp` — an example of
  choosing an EGL/GL manager, `RasterizerGLES3::make_current(...)`,
  registering a display backend and defining swap behavior.
- `drivers/gles3/rasterizer_gles3.cpp` — upstream GLAD/EGL symbol loading,
  GL-over-GLES configuration, renderer setup.
- `servers/display/display_server_headless.cpp` — existing dummy
  backend, **not** a graphical implementation.

## 2. PS5 DisplayServerPS5

Implement `engine/platform/ps5/display_server_ps5.{h,cpp}` with
the real Godot 4.7.2 `DisplayServer` interface. At minimum:

- Register `ps5` backend through `DisplayServer::register_create_function`.
  Fail with a useful `Error` and log if EGL startup fails.
- Own exactly one main window and return actual `1920×1080` size (or
  queried mode), usable rect, focus status, DPI, display refresh and
  window drawing state. Do not return fictitious success for unsupported
  subwindows or native handles.
- Create the EGL display/context/surface before constructing the GLES3
  compositor; choose the GL-over-GLES mode suited to the PS5 Mesa
  **OpenGL 4.6 Core Profile** path (`RasterizerGLES3::make_current(true)`
  should be tested against Godot's actual GLAD path).
- Ensure that `glGetString(GL_RENDERER)` still reports `PS5 AGC` after
  context transfer to the Godot renderer; expose detailed diagnostics.
- Own `swap_buffers`/presentation, thread affinity and orderly EGL
  teardown. Destroy Godot GL objects **before** destroying EGL context.
- Bind the existing `ps5_pad::Bridge` to Godot's event loop without
  duplicate input delivery. Preserve the 38-X-press regression fixture.

## 3. Independent integration build

Provide a separate experimental signed PS5 title / unique Title-ID,
not overwriting `PPSA99843` or `PPSA99844`.
CI must build *upstream Godot* with `GLES3_ENABLED`, link its actual
rendering libraries, package a real Godot-created `.pck`, and run
host-side binary, ZIP, metadata and linkage validations. If compilation
fails, keep it an Actions diagnostic, not a TESTBUILD release.

Any GPL-linked graphics binaries must be distributed with compatible
licenses and corresponding sources. Review the OpenGL SDK mirror first.

## 4. Hardware acceptance gates

1. Confirm the same Godot PS5 binary prints an actual
   `DisplayServerPS5` initialization marker and renderer identity.
2. Render a **Godot-generated** packed scene with `Sprite2D`,
   `ColorRect`, `Label`, animation or rotation from GDScript.
3. Verify scene changes when X is pressed via the Godot Input singleton;
   capture real PS5 screenshot and engine logs.
4. Validate graceful exit, context cleanup, resource loading and user://
   save writes; check repeated launches.

`GL_TRIANGLE_SUBMITTED` and `SPRITE_2D_DRAW_SUBMITTED` from the
stand-alone GPU probe alone are **not** evidence of a functional Godot
DisplayServer.
