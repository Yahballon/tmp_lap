# M23 – Find hidden GL driver waits after M22 did not touch its supposed resource cap

Date: 2026-10-24. Directly extracted actual firmware-5.10 user archive `bfpilot-download (42).tar`, specifically the LAST `GODOT_PS5_M22_BOUNDED_CANVAS_RING active=1` session, not older append-only history.

## Real physical evidence

- The native PS5 v0.2.6 title boots with M22 active and `PACK_SOURCE=BUNDLED_VISUAL` (the signed built-in Godot Pong).
- Performance regresses exactly as before: an early 5-second report shows 84 frames, later 4,2,2,1,1. Largest `GODOT_PS5_M17_PHASE_5S max_draw_ms` ~9,543.73ms. Main loop total ~11,501ms while actual thread CPU was ~21.54ms. EGL swap ~0.1ms. The selected **original M19 23 GL functions total ~1.89ms** in that late frame.
- **ZERO `GODOT_PS5_M22_CANVAS_RING cap=9 ... orphan_reuses=N` lines** in the entire appended stdout log. The cap/GL orphaning branch in M22 is therefore never demonstrated as having executed. The original theory that six `glBufferData` calls each frame demonstrate indefinite growth of **canvas ring buffer names** was unproven and is now substantially weakened. Normal per-frame GL buffer replacement can account for those calls.
- `glClientWaitSync` still ~0ms, previously disabled GPU timestamp queries, dummy AudioOut, VSync/off, Godot sleep, and alternate platformer/Pong PCK all failed to cure the massive waits. Do not reapply these as universal fixes.
- Original stderr has recurring nonfatal project.binary/global-class cache missing-resource messages; it shows no conclusive GPU OOM or crash in the latest session. AudioOut continues running. No indication that CPU-bound GDScript/Physics2D is responsible for the severe render wait.

## M23 strategy and source

Branch `perf/godot-ps5-m23-missing-gl-waits` **starts from M21** and therefore does **NOT** retain M22's speculative nine-buffer cap or orphan/reuse change. The experimental PS5 GLES3 engine and native renderer otherwise remain identical to the M21 branch.

M23 collects monotonic per-API timing from **32 additional** actual OpenGL functions not covered by existing M19's 23-call trace: glBindTexture, glActiveTexture, glBindVertexArray, glGenBuffers, glDeleteBuffers, glGetSynciv, glDeleteSync, glGetProgramiv, glGetShaderiv, glTexParameteri, glEnable/Disable, vertex attribute functions, glBindBufferBase/Range, viewport/scissor, framebuffer functions, and more. All functions delegate their original arguments and return values **exactly once**, and the existing GLAD/EGL getProcAddress resolver is preserved. The header `m23_gl_trace_extra.hpp` is packaged with the native DevBridge source by `stage_native_link.py`.

Markers: `GODOT_PS5_M23_EXTRA_GL_TIMING active=1 methods=32`; every 5 seconds `GODOT_PS5_M23_GL_PROFILER_READY`, `GODOT_PS5_M23_GL_5S name=X calls=N total_ms=N max_call_ms=N spikes_80ms=N` and `GODOT_PS5_M23_GL_SUMMARY_5S`, alongside original M19 `GL_SUMMARY_5S` and M17 phase profiler. No additional GL draw, sync, frame pacing, logging per individual call, shader or PCK change.

Potential alternative causes if all 32 are also fast: GL entry points outside both sets, a shader/process mutex, driver command queue completion in non-GL wrappers, renderer callbacks or free-running GPU/AGC scheduling. Could involve firmware 5.10-specific GPU driver behavior. The pinned unofficial SDK mirror is **v1.0.2**, also used by previous versions; PS5-opengl SDK 1.0.2 explicitly allows submissions to take up to 30 seconds to retire before fail-stop. It must be investigated with driver-specific timings rather than blaming Godot scene logic. Independent desktop Mesa bugfix releases cannot automatically repair this PS5 SDK's custom AGC port.

## Next hardware acceptance

Signed app `PPSA99847`, version `0.2.7`, contentVersion `01.000.018`. Dedicated trusted-branch PS5 native Build + automatic TESTBUILD publisher, verified actual signed ELF/ZIP. Host tests and compilation do NOT imply performance fixed.

On PS5: keep `present_mode.txt` on `vsync`, restore normal audio, temporarily rename `launch.txt` to test bundled Pong for ~45 seconds (or until it freezes). Download/export `gles3-stdout.log` and `gles3-runtime.log`; analyze the **new session only**. Compare all `M23_GL_5S` and `M19_GL_5S` timing totals to the corresponding `M17_PHASE_5S draw_ms` to distinguish an unprofiled blocking call from driver-internal waiting. Only change actual driver code after a culprit has been found. No direct RAM-leak claim.
