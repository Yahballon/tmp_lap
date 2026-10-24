# M20 – Avoid synchronous GPU timestamp profiling queries in experimental PS5 GLES3

Date: 2026-10-24. Physical console firmware 5.10, archive `bfpilot-download (39).tar`, M19 v0.2.3. **Diagnostic hypothesis; performance is NOT yet confirmed.**

## Reproducible console evidence

Both newest M19 runtime sessions boot the bundled Pong `PACK_SOURCE=BUNDLED_VISUAL` at **1920×1080**, with `GODOT_PS5_M19_GL_PROFILER_READY hooked=25 names=23`, VSync=1 and native AudioOut.

- Session 1: M19 first five-second window shows **6 GL calls above 80 ms**, with `glClear` max 644.58ms and instanced draws ~273ms; M19 first-window aggregate 2066.10ms. Session 2 repeats very similarly: `glClear` 644.21ms, instanced draws ~274ms, aggregate 2054.82ms.
- After warm-up the **same monitored 23 GL calls** are individually fast (last five-second window maximum **1.43ms**) while Godot `M17_PHASE_SPIKE draw_ms` reaches **18,221.33ms** in session 2 (about 8.6sec in the analogous session 1). M13 `egl_swap_ms` stays ~0.1ms. M18 `Main::iteration` clock reports only ~20ms of CPU time in a 10.5-second frame. M16 native sleep calls=0.
- Therefore no individual *M19-instrumented* GL API call continuously explains the stall. **Uninstrumented API calls and asynchronous GPU waits remain possible**.

## Source-level candidate identified

The pinned original **Godot 4.7.2** `drivers/gles3/storage/utilities.cpp` unconditionally executes `glQueryCounter(..., GL_TIMESTAMP)` from GLES3 `Utilities::capture_timestamp` and later `glGetQueryObjectui64v(..., GL_QUERY_RESULT, &temp)` from `Utilities::_capture_timestamps_begin()` when running desktop OpenGL. This requests the GPU result synchronously, potentially blocking the CPU if the GPU queue lags. **Neither function is in the original M19 set of 23 instrumented GL API wrappers**.

M20's guarded patch `scripts/patch_ps5_m20_gl_timestamps.py` conditionally removes **only these two GPU timing calls** when both `PS5_ENABLED` and `GODOT_PS5_NATIVE_GLES3` are defined. Godot's CPU timestamp bookkeeping and all original rendering, sync, physics, input and audio operations remain unchanged. This removes GPU timing information from Godot's *internal diagnostic profiler only* for this experimental PS5 target. Other platforms and normal source remain unaffected. Guard validates the two pinned upstream anchors and is applied through `scripts/build-engine.sh` only with `GODOT_PS5_M20_DISABLE_GL_QUERIES=1`.

M20 native app logs `GODOT_PS5_M20_GPU_TIMESTAMP_SYNC_DISABLED active=1` at startup, retains M13–M19 measurements. No VSync changes, no artificial sleeps or `glFinish`, no PCK modifications.

## Acceptance / next user test

Native cross-build/signing must succeed; TESTBUILD publication requires verified trusted-push artifact, `PPSA99847` v0.2.4 / content version `01.000.015`. Existing v0.2.3 remains as rollback.

1. Restore normal audio (`m18_audio_mode.txt` absent). Keep `present_mode.txt` absent or `vsync`.
2. Temporarily leave `launch.txt` renamed to load **bundled Pong** (`PACK_SOURCE=BUNDLED_VISUAL`); verify M20 startup marker; play at least 60 seconds, compare last 30 sec actual FPS/spike counts to v0.2.3.
3. If Pong stays consistently smooth, restore `launch.txt` pointing to `m12d-platformer-asset2201.pck` and test Level_01/02; confirm native sound and collision remain functional.
4. If still unplayable, do **not** claim fixed: focus next on `glGetQueryObjectui64v` and other uninstrumented API timing, GPU fence retirement, renderer driver queue timing, or PS5 main thread descheduling.

Physical success or root-cause attribution is **not yet verified**. Do not merge as generally stable without it.
