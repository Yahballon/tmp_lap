# M22 – Bounding PS5/Godot GLES3 Canvas Instance Buffers

Date: 2026-10-24; reference firmware 5.10, real user TAR `bfpilot-download (41).tar`.

## Evidence

Newest session definitely starts the M21 v0.2.5 title (`GODOT_PS5_M21_CANVAS_FENCE_NONBLOCKING active=1`), built-in Pong (`GODOT_PS5_PONG_READY` / `PACK_SOURCE=BUNDLED_VISUAL`), real PS5 AGC / Mesa 26.2.0 at 1920×1080, VSync=1 and native AudioOut enabled. The first M17 window has **88 frames**, later windows 4, 2, 2, 1, 1. Render phase reaches 9.54s, and `M18_MAIN_CPU_SPIKE` shows 11.49s elapsed vs only 20.97ms actual thread CPU in a late frame. `egl_swap_ms` is ~0.1ms. `glClientWaitSync` is now called as expected from M21, but only takes ~0.01ms, so the reported stall is elsewhere.

**New strong correlation:** the same session has `GODOT_PS5_M19_GL_5S name=glBufferData calls=516` while only 86 frames were rendered in the corresponding 5-second interval; this is about **6 GL buffer-storage updates per frame**, and later intervals have six even when rendering just one frame. The pinned original Godot Canvas renderer's `_allocate_instance_data_buffer()` can **insert a fresh set of three buffer names indefinitely** if the previous frame's GL fences remain unsignaled, with batch UBOs as large as ~2MiB each. The PS5 Mesa AGC log declares a 268MiB GPU arena (and 320MiB shared pool). If the 2D buffer ring continuously grows, exhausting/churning GPU memory is a credible cause of the initial playable frames followed by near-total stops.

**Important uncertainty:** GL buffer calls are not proof of unique buffer object allocations or currently resident GPU bytes, and the GL driver's actual freed backing stores have not been measured. The M22 implementation below is an experimental A/B workaround; do **not** label this proven leak repair until the actual device is tested.

## Scoped change

New M22 v0.2.6 patch `patch_ps5_m22_bounded_canvas_ring.py`, enabled only for the signed experimental `GODOT_PS5_NATIVE_GLES3` build by `GODOT_PS5_M22_BOUNDED_RING=1`:
- The original first 9 Canvas2D data-buffer sets can be allocated as before. After that, **no more GL buffer names are inserted by `_allocate_instance_data_buffer`**.
- Each subsequent allocation request advances within the existing ring, deletes only the superseded GL sync *handle*, and **orphans each existing `GL_ARRAY_BUFFER` and relevant `GL_UNIFORM_BUFFER` storage** with `glBufferData(...,nullptr,GL_STREAM_DRAW)` before Godot's subsequent mapped writes. This is conforming GL's method to preserve GPU references to old backing storage, rather than writing into a live buffer object in place.
- Logs `GODOT_PS5_M22_CANVAS_RING cap=9 live_sets=9 orphan_reuses=N` on first and each 60th reuse; launch marker `GODOT_PS5_M22_BOUNDED_CANVAS_RING active=1 max_sets=9 orphan=1`.
- Original GLES3 draw calls, shader programs, VSync, PCK, GDScript, Physics2D, DualSense, native AudioOut, M13–M21 diagnostics, and non-PS5 platforms are unchanged.

**Risk:** orphaning is safe in a conforming GL driver, but the experimental PS5 GPU driver may still retain backing allocations while GPU commands are in flight. This change bounds GL object growth, **not** the driver's internal storage. If the GPU commands never retire it may still stall. GL buffer deletion / fence retirement are not solved globally. Test performance and visual correctness; retain signed v0.2.5 and v0.2.4 rollback.

## Hardware acceptance gate

Native PS5 SDK compilation must succeed, signed `PPSA99847` v0.2.6 (`contentVersion=01.000.017`) built via dedicated GitHub Actions, then TESTBUILD publication only on verified trusted `push` workflow success. On PS5 keep `present_mode.txt` absent/`vsync`, `m18_audio_mode.txt` absent (normal audio), and temporarily rename `launch.txt` to run built-in Pong for 90+ seconds. Verify two M22 markers and compare M17 frame counts and M13 swaps: the game must remain responsive *well past the previous ~88-frame slowdown*, not just begin well. Also test original `m12d-platformer-asset2201.pck` levels 01/02 and check sound, collision and image corruption. **No performance success claim until physical PS5 testing.**
