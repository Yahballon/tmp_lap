# M21 – Nonblocking GLES3 Canvas2D Fence Status Probe (experimental)

Date: 2026-10-24. Real PS5 hardware archive `bfpilot-download (40).tar`, firmware 5.10, M20 v0.2.4.

## Hardware evidence

The latest session **definitely runs M20 v0.2.4**: stdout includes `GODOT_PS5_M20_GPU_TIMESTAMP_SYNC_DISABLED active=1` and `GODOT_PS5_M19_GL_PROFILER_READY`; built-in Pong `GODOT_PS5_PONG_READY`, `PACK_SOURCE=BUNDLED_VISUAL`, `GODOT_PS5_M14_PRESENT_MODE requested=vsync effective=vsync`, original PS5 AudioOut enabled.

The original Godot-4.7.2 Pong scene begins with ~90 frames in a five-second window, but soon drops to **1 or 2 frames** with `M17 draw_ms` >21 seconds and later **48.0 seconds** on an individual frame. Main::iteration wall time reaches **57.68s** for just **20.98ms actual CPU time**; the `egl_swap_ms` remains ~0.1ms. Final M19 aggregate selected 23 GL calls is just **1.87ms** in that same massive frame, with max individual selected call ~1.43ms. Native AudioOut still logs delivered sound buffers. M16 sleep remains 0 calls. The timestamp-query bypass therefore **did NOT fix hardware freezes**.

The first window has six relatively slow draw calls, but they are not the recurring multi-second stalls.

## Identified previously unprofiled canvas GL sync status query

Pinned upstream Godot 4.7.2 `drivers/gles3/rasterizer_canvas_gles3.cpp`, within `RasterizerCanvasGLES3::canvas_render_items()`, executes `glGetSynciv(fence,GL_SYNC_STATUS,...)` each time a rotating canvas instance data buffer has a fence. This GL entry point was **not in the 23 M19 wrappers**. Source also has a conditional `glClientWaitSync(fence,0,100000000)` to wait up to 100ms for older-than-two-frame fences, and a path to allocate new instance buffers when recent fences are still unsignaled. M19 recorded `glFenceSync` per frame, but did not record glClientWaitSync activity in the worst windows. This is consistent with the GET_SYNC_IV call blocking until GPU completion, but **it is not yet measured or proven**. The separate M20 sync timestamp hypothesis was contradicted by real hardware.

## M21 code

Experimental v0.2.5 branch `perf/godot-ps5-m21-nonblocking-canvas-fence` patches the exact single query of pinned original Godot code. **PS5 native GLES3 only**: replace `glGetSynciv` with `glClientWaitSync(fence,0,0)`, i.e. zero-timeout probe. `GL_ALREADY_SIGNALED` or `GL_CONDITION_SATISFIED` map to `GL_SIGNALED`; `GL_TIMEOUT_EXPIRED` maps to `GL_UNSIGNALED`, preventing unsignaled fences from being marked ready at the status check; on `GL_WAIT_FAILED` the original `glGetSynciv` is used as a conservative fallback. The remainder of the original resource-ring management and draw code stays **identical**, including the upstream older-fence wait policy (which should be separately audited if this test succeeds). All other platforms retain the original status query. Thus the experiment changes GL status polling semantics and might increase the frequency of downstream timeout paths; it must be validated on real hardware.

The existing M19 wrapper already logs `glClientWaitSync` durations, so after the patch look for `GODOT_PS5_M19_GL_5S name=glClientWaitSync ...`. The launcher prints `GODOT_PS5_M21_CANVAS_FENCE_NONBLOCKING active=1`.

M13–M20 probes, original GLES3 draw calls, VSync=1, external PCK, Physics2D, GDScript, native AudioOut and DualSense remain unchanged. No forced GPU completion or global driver hook.

## Native signing / user acceptance

Host tests patch the pinned original Godot source and reapply idempotently. Signed package `PPSA99847` version `0.2.5` / contentVersion `01.000.016` from isolated M21 GitHub Actions `push` workflow; trusted successful CI alone permits TESTBUILD publication.

Test **bundled Pong** with `launch.txt` absent/renamed, `present_mode.txt` absent or `vsync`, native audio default. Test **at least 60 seconds**, especially after initial ~90-frame burst; inspect whether M17 Draw remains normal and M19 glClientWaitSync spike count / GPU sync wait time. Then test original M12d platformer. Older signed v0.2.4 and v0.2.3 releases remain valid rollback. **This is a hypothesis, not an observed performance fix.**
