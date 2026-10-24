# M19 – Native PS5 GLES3 OpenGL call-path profiler

Date: 2026-10-24. Physical PS5 hardware log archive: `bfpilot-download (38).tar` (Godot 4.7.2 experimental v0.2.2, firmware 5.10).

## Physical PS5 A/B results

The last three sessions of `gles3-stdout.log` confirm both requested tests actually happened:

| Session | Mode / PCK | Result |
| --- | --- | --- |
| 1 | `GODOT_PS5_M18_AUDIO_AB_MODE mode=native_audioout`, M12d platformer | Severe delays; M17 draw reaches **7700ms** in a sample; EGL swap ~0.1ms |
| 2 | `GODOT_PS5_M18_AUDIO_AB_MODE mode=dummy`, M12d platformer | Remains badly laggy; M15 wall up to **1983ms** per frame despite no AudioOut worker |
| 3 | `GODOT_PS5_M18_AUDIO_AB_MODE mode=native_audioout`, `PACK_SOURCE=BUNDLED_VISUAL` | The **built-in Pong** PCK, explicitly copied into `visual-smoke.pck` by CI, also stutters after initially running faster. Physical log contains `GODOT_PS5_PONG_READY`, M17 `scene_ms` usually ~0.15ms, `draw_ms` up to **1031ms**; M18 main-thread CPU work often under 10ms. No Godot native sleep calls in these final windows. The bundled-PCK test thus disproves an M12d-only slowdown. |

Two first-party controls were verified in the log, not inferred from filename. `launch.txt.bak` contains the working M12d path; the user correctly restored/renamed it. This test is stronger than an unverified claim that merely removing the file improves FPS.

This does **not** prove a particular GL function, CPU scheduler fault or AudioOut bug. It narrows expensive waits to the original Godot GLES3 render flow or the PS5 GL/GPU driver stack. The renderer logged `PS5 AGC` / `Mesa 26.2.0`. The external PS5 OpenGL project explicitly notes that CPU fallback and synchronous operations remain in some paths, even when API support is present; that statement does not establish a causal explanation for this specific build.

## Diagnostic change

M19 v0.2.3 instruments the exact *original* GL resolver: `eglGetProcAddress`, which Godot GLAD obtains from the native graphics hook. For **23 selected original OpenGL API entries**, wrappers collect monotonic call durations and forward the original parameters/return values unchanged. No extra GPU commands, fences, glFinish, texture uploads, framebuffer readbacks, or gameplay changes are introduced.

New markers: `GODOT_PS5_M19_GL_PROFILER_READY hooked=N names=23`; every 5s `GODOT_PS5_M19_GL_5S name=<api> calls=N total_ms=N max_call_ms=N spikes_80ms=N`; one `GODOT_PS5_M19_GL_SUMMARY_5S` with overall coverage. The existing M13–M18 observations remain.

M19 does add two monotonic clock calls around each *instrumented* API; it is **diagnostic, not an FPS fix**. A stall in an uninstrumented GL call, a driver internal worker, resource-loader mutex or external scheduling path will not be attributed to a single wrapped function. We must compare the aggregate recorded GL durations to `M17_PHASE_5S draw_ms` and then extend only missing categories. An apparent zero-time result is not proof all rendering is free.

## Test / rollback

Package `PPSA99847` version `0.2.3` contentVersion `01.000.014` is built by `build-godot-ps5-m19-gl-profiler.yml` (public SDK, signed title) and released to TESTBUILD only from verified trusted successful push CI. Preserve v0.2.2 signed release for rollback.

Preferred hardware test: keep `present_mode.txt` absent or `vsync` (never default immediate); leave `m18_audio_mode.txt` absent for normal sound. Run built-in Pong first (`launch.txt` temporarily renamed) for 60+ sec; export fresh `gles3-stdout.log`, `gles3-stderr.log`, `gles3-runtime.log`. Restore `launch.txt` to play the M12d platformer; compare. Do not call the lag fixed absent console acceptance.
