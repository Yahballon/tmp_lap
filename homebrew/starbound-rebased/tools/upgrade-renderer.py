#!/usr/bin/env python3
"""Patch the pinned SDL2 Gold872 renderer for fullscreen PS5 and HD source sprites.

Run after adapt-ps5-engine.py and resolve-resource-paths.py. Gameplay, input,
camera, physics and world-coordinate collision rectangles remain unchanged.
"""
from pathlib import Path
import sys

root = Path(sys.argv[1]) if len(sys.argv)>1 else Path('build/gold872-engine')

def patch(filename, old, new):
    p=root/filename
    s=p.read_text()
    count=s.count(old)
    if count != 1:
        raise RuntimeError(f'{filename}: expected one match, got {count}: {old[:90]}')
    p.write_text(s.replace(old,new,1))

patch('src/TextureManager.cpp',
'''   window = SDL_CreateWindow("Starbound Rebased", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             SCREEN_WIDTH, SCREEN_HEIGHT,
                             SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);''',
'''#ifdef __FreeBSD__
   // Full-resolution console output, retaining the original 800x480 game space.
   window = SDL_CreateWindow("Starbound Rebased", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             1920, 1080, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
   if (!window) {
      sdl_boot_mark("TM07a fullscreen mode failed; trying window fallback");
      window = SDL_CreateWindow("Starbound Rebased", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                1280, 720, SDL_WINDOW_SHOWN);
   }
#else
   window = SDL_CreateWindow("Starbound Rebased", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             SCREEN_WIDTH, SCREEN_HEIGHT,
                             SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
#endif''')

patch('src/TextureManager.cpp',
 'TextureManager TextureManager::instance;',
 'TextureManager TextureManager::instance;\nstatic SDL_Texture* sbBackgroundTexture = nullptr;')

patch('src/TextureManager.cpp',
 '   SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);',
'''#ifdef __FreeBSD__
   // No letterboxing: stretch exact engine world units to the whole framebuffer.
   SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // crisp nearest-neighbour
   SDL_RenderSetLogicalSize(renderer, 0, 0);
   int rw=0, rh=0;
   if (SDL_GetRendererOutputSize(renderer, &rw, &rh)==0 && rw>0 && rh>0)
      SDL_RenderSetScale(renderer, float(rw)/float(SCREEN_WIDTH),
                                  float(rh)/float(SCREEN_HEIGHT));
   else {
      sdl_boot_mark("TM12a SDL_GetRendererOutputSize failed");
      SDL_RenderSetScale(renderer, 1.6f, 1.5f);
   }
#else
   SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
#endif
   SDL_Surface* sky = IMG_Load(SB_ASSET_PATH("res/sprites/backgrounds/alien_sky.png"));
   if (sky) {
      sbBackgroundTexture = SDL_CreateTextureFromSurface(renderer, sky);
      SDL_FreeSurface(sky);
   }
   sdl_boot_mark(sbBackgroundTexture ? "TM12b sky loaded" : "TM12b sky missing");''')

patch('src/TextureManager.cpp',
 '   SDL_DestroyRenderer(renderer);',
 '   if (sbBackgroundTexture) { SDL_DestroyTexture(sbBackgroundTexture); sbBackgroundTexture=nullptr; }\n   SDL_DestroyRenderer(renderer);')

patch('src/TextureManager.cpp',
 '   SDL_RenderFillRect(renderer, &backgroundRect);',
'''   if (currentColor == BackgroundColor::BLUE && sbBackgroundTexture)
      SDL_RenderCopy(renderer, sbBackgroundTexture, nullptr, &backgroundRect);
   else
      SDL_RenderFillRect(renderer, &backgroundRect);''')

patch('src/systems/RenderSystem.cpp',
'''      TextureManager::Get().Draw(texture->getTexture(), spritesheet->getSourceRect(),
                                 destinationRect, texture->isHorizontalFlipped(),
                                 texture->isVerticalFlipped());''',
'''      SDL_Rect hdFrame = spritesheet->getSourceRect();
      hdFrame.x *= 2;
      hdFrame.y *= 2;
      hdFrame.w *= 2;
      hdFrame.h *= 2;
      TextureManager::Get().Draw(texture->getTexture(), hdFrame,
                                 destinationRect, texture->isHorizontalFlipped(),
                                 texture->isVerticalFlipped());''')

print("Applied fullscreen, high-detail 2x source sprites and original sky backdrop")
