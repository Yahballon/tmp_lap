# ZQuestClassic PS5 — Indexed Bitmap to EGL/OpenGL bridge

This experimental submodule is part of the **full zplayer port**. It is
**not playable**. The branch tests a missing platform primitive: uploading
8-bit indexed, palette-based game frames to the PS5's existing EGL/OpenGL
graphics stack at runtime, with no Nintendo art/assets.

## Backend decision

The SDL2 PS5 fork examined is
[OpenAGC/SDL2](https://github.com/OpenAGC/SDL2/tree/ee4c47dc0d617b3bc8f35108f9956baf228a1322)
(revision `ee4c47dc0d617b3bc8f35108f9956baf228a1322`).
Its PS5 video backend directly accesses `sceVideoOut`, PS5 direct memory,
`kevent` queues and the fork's video-driver structures. Its optional GL
implementation dynamically opens `libOSMesa.so.8` and assumes OSMesa, not
the working `PS5-DevBridge` EGL context provided by
[ps5-opengl](https://github.com/LedyBacer/ps5-opengl).

Consequently, neither an arbitrary SDL2 library nor its OSMesa context
can be assumed to work with the current launcher. SDL input/audio are
separate tasks. This video proof directly reuses the **already working**
PS5 native EGL/OpenGL `GlBatch` and `DrawList` code, as an adapter layer.

## Implemented bridge

- `paletted_surface.hpp`: indexed 8-bit frame, pitch validation, 256
  RGB6 palette entries (Allegro colors 0..63), stable RGBA8 conversion,
  dynamic palette changes, bounds and size limits. Includes source-buffer
  length validation; safe for pitched legacy `BITMAP` memory.
- `ps5_palette_texture.cpp`: one allocated GL_RGBA8 texture, reused via
  `glTexSubImage2D` rather than reallocating every frame, nearest-neighbor
  image filter to keep pixel art crisp.
- `ps5_video_probe.cpp`: clearly labeled 256x224 procedural moving
  test card visible inside native UI (separate diagnostic artifact).
- `paletted_surface_test.cpp`: host tests for 6-bit -> 8-bit conversion,
  oversized pitch, buffer truncation, dynamic palette changes and bounds.
- `inject_video_probe.py`: stages a copy of native UI in a separate
  engine-only GitHub Actions job. Does not change public frontend source or
  `version.json`.

**Test result criteria:** host test success, PS5 ELF cross-compilation,
packaged ZIP verification, and marker `PS5_INDEXED_VIDEO_BRIDGE_READY`
present in ELF. The actual animated pattern's appearance/refresh and
performance still need console validation; the CI job cannot prove that.

## Remaining for actual gameplay

1. Wire original Allegro-Legacy `BITMAP` frame buffer/row pitches and
   palette changes into this adapter. A procedural preview is **not**
   original game rendering.
2. Integrate original `zplayer` entrypoint, quest parser and ZASM VM.
3. Port input events (DualSense -> Allegro), Allegro timer/event queue,
   voice/audio mixer and resource loading/writable saves.
4. Run an actual legally held quest and verify graphics, movement,
   audio, save/load and exit on jailbroken PS5 hardware.
5. Only then ship an explicitly playable release.

All original source attribution remains with ZQuestClassic and
PS5-DevBridge; no copyrighted Nintendo game media included.
