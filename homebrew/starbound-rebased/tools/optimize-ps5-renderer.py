#!/usr/bin/env python3
"""PS5 software-renderer performance fix for the Gold872 engine.

The v0.1.9 renderer scaled EVERY sprite (often with CPU SDL_RenderCopyEx)
to the 1920x1080 window. The PS5 SDL2 port falls back to a software renderer,
which supports SDL_RENDERER_TARGETTEXTURE. Draw at the native 800x480 game
resolution into a render-target texture, then upscale the complete frame once.

Apply AFTER upgrade-renderer.py, BEFORE copying patched sources into app/src.
No changes to the physics, input, collision coordinates or game logic.
"""
from pathlib import Path
import sys

root=Path(sys.argv[1]) if len(sys.argv)>1 else Path('build/gold872-engine')

def replace_one(name, old, new):
    path=root/name
    s=path.read_text()
    n=s.count(old)
    if n != 1:
        raise RuntimeError(f'{name}: expected exactly one match ({n}): {old[:110]!r}')
    path.write_text(s.replace(old,new,1))

replace_one('src/TextureManager.cpp',
    'static SDL_Texture* sbBackgroundTexture = nullptr;',
    '''static SDL_Texture* sbBackgroundTexture = nullptr;
static SDL_Texture* sbSceneTarget = nullptr;
#ifdef __FreeBSD__
// Only the final composed frame is scaled to Full HD.
static Uint32 sbPerfLastTick = 0;
static unsigned sbPerfFrameCount = 0;
#endif''')

replace_one('src/TextureManager.cpp',
    '''   sdl_boot_mark(sbBackgroundTexture ? "TM12b sky loaded" : "TM12b sky missing");''',
    '''   sdl_boot_mark(sbBackgroundTexture ? "TM12b sky loaded" : "TM12b sky missing");
#ifdef __FreeBSD__
   // The PS5 backend currently uses the software SDL renderer.
   // Renderer-to-texture is supported by SDL's software renderer, even when
   // hardware-accelerated SDL_CreateRenderer() was unavailable.
   SDL_RendererInfo info = {};
   if (SDL_GetRendererInfo(renderer, &info) == 0 &&
       (info.flags & SDL_RENDERER_TARGETTEXTURE)) {
      sbSceneTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888,
                           SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);
      if (sbSceneTarget && SDL_SetRenderTarget(renderer, sbSceneTarget) != 0) {
         SDL_DestroyTexture(sbSceneTarget);
         sbSceneTarget = nullptr;
      }
   }
   if (sbSceneTarget) {
      SDL_SetTextureBlendMode(sbSceneTarget, SDL_BLENDMODE_NONE);
      SDL_RenderSetScale(renderer, 1.0f, 1.0f);
      sdl_boot_mark("PERF01 native 800x480 offscreen target enabled");
   } else {
      // Don't abort app startup if this SDL port does not expose targets.
      sdl_boot_mark("PERF01 render target unavailable; using original path");
   }
   sbPerfLastTick = SDL_GetTicks();
#endif''')

replace_one('src/TextureManager.cpp',
   '''   if (sbBackgroundTexture) { SDL_DestroyTexture(sbBackgroundTexture); sbBackgroundTexture=nullptr; }
   SDL_DestroyRenderer(renderer);''',
   '''   if (sbSceneTarget) {
      SDL_SetRenderTarget(renderer, nullptr);
      SDL_DestroyTexture(sbSceneTarget);
      sbSceneTarget=nullptr;
   }
   if (sbBackgroundTexture) { SDL_DestroyTexture(sbBackgroundTexture); sbBackgroundTexture=nullptr; }
   SDL_DestroyRenderer(renderer);''')

replace_one('src/TextureManager.cpp',
    '''void TextureManager::Clear() {
   // Creates black borders''',
    '''void TextureManager::Clear() {
#ifdef __FreeBSD__
   if (sbSceneTarget) {
      // Keep entity coordinates in original 800x480 pixels.
      if (SDL_GetRenderTarget(renderer) != sbSceneTarget)
         SDL_SetRenderTarget(renderer, sbSceneTarget);
      SDL_RenderSetScale(renderer, 1.0f, 1.0f);
   }
#endif
   // Creates black borders''')

replace_one('src/TextureManager.cpp',
    '''void TextureManager::Display() {
   SDL_RenderPresent(renderer);
}''',
    '''void TextureManager::Display() {
#ifdef __FreeBSD__
   if (sbSceneTarget) {
      // Render native resolution for world sprites/HUD, then scale only once.
      SDL_SetRenderTarget(renderer, nullptr);
      SDL_RenderSetScale(renderer, 1.0f, 1.0f);
      SDL_RenderSetViewport(renderer, nullptr);
      int width = 0, height = 0;
      if (SDL_GetRendererOutputSize(renderer, &width, &height) == 0 &&
          width > 0 && height > 0) {
         SDL_Rect screen = {0, 0, width, height};
         SDL_RenderCopy(renderer, sbSceneTarget, nullptr, &screen);
      }
      SDL_RenderPresent(renderer);
      // Re-select native-resolution canvas for next game frame.
      SDL_SetRenderTarget(renderer, sbSceneTarget);
      SDL_RenderSetScale(renderer, 1.0f, 1.0f);
   } else {
      SDL_RenderPresent(renderer);
   }

   // Periodic log measured FPS, not per-frame file writes.
   ++sbPerfFrameCount;
   Uint32 now = SDL_GetTicks();
   Uint32 elapsed = now - sbPerfLastTick;
   if (elapsed >= 5000) {
      char result[128];
      unsigned fps10 = (unsigned)((sbPerfFrameCount * 10000ULL) / (elapsed ? elapsed : 1));
      std::snprintf(result, sizeof(result),
                    "mode=%s fps=%u.%u frames=%u ms=%u",
                    sbSceneTarget ? "offscreen" : "direct",
                    fps10 / 10, fps10 % 10, sbPerfFrameCount, elapsed);
      sdl_boot_mark(result);
      sbPerfFrameCount = 0;
      sbPerfLastTick = now;
   }
#else
   SDL_RenderPresent(renderer);
#endif
}''')

replace_one('src/TextureManager.cpp',
    '''   if (!horizontal && !vertical) {
      SDL_RenderCopyEx(renderer, texture.get(), &sourceRect, &destRect, 0, nullptr, SDL_FLIP_NONE);
      return;
   }''',
    '''   if (!horizontal && !vertical) {
      // Avoid the expensive software RenderCopyEx path when no flipping is needed.
      SDL_RenderCopy(renderer, texture.get(), &sourceRect, &destRect);
      return;
   }''')

print("PS5 software renderer now renders each scene at 800x480 and upscales once")
