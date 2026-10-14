# Starbound Rebased v0.1.10 — PS5 fullscreen performance fix

v0.1.9 introduced a noticeable gameplay slowdown on real PS5 hardware. The console SDL2 video driver falls back to the software renderer. Scaling hundreds of sprites individually into a 1920x1080 framebuffer is expensive, and Gold872 gameplay advances once per drawn frame, making jumps feel floaty or like slow motion when FPS falls.

## Changes
- Draw all game tiles, sprites and the UI at the engine's native 800x480 resolution into one offscreen SDL render-target texture.
- Blit/upscale only that finished frame to the 1920x1080 display.
- Use SDL_RenderCopy instead of SDL_RenderCopyEx for unflipped entities.
- Keep the original physics, animation code, movement speeds, HD atlas source rectangles, backgrounds and ShadowMount Title ID `PPSA99642`.
- Log an averaged measured FPS every ~5 seconds in `/data/starbound-rebased/boot.log`, tagged `mode=offscreen fps=...` to verify performance on hardware.
- If the SDL renderer cannot create a target texture, log `PERF01 render target unavailable` and fall back without crashing.

This is a **candidate performance fix**, not a verified 60 FPS claim. Test v0.1.10 directly on PS5 and check the FPS telemetry. v0.1.9 remains available for rollback.
