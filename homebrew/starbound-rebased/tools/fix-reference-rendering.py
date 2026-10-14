#!/usr/bin/env python3
"""Fix unframed textures and opt-in to the user-provided HD sky when installed."""
from pathlib import Path
import sys

root=Path(sys.argv[1]) if len(sys.argv)>1 else Path("build/gold872-engine")
def patch(path,old,new):
    p=root/path
    s=p.read_text()
    if s.count(old)!=1:
        raise RuntimeError(f"{path}: expected one match for {old[:100]!r}, got {s.count(old)}")
    p.write_text(s.replace(old,new,1))

patch("src/systems/RenderSystem.cpp",
    '''   } else {
      TextureManager::Get().Draw(
          texture->getTexture(), SDL_Rect{0, 0, position->scale.x, position->scale.y},
          destinationRect, texture->isHorizontalFlipped(), texture->isVerticalFlipped());
   }''',
    '''   } else {
      // Unframed icons and menus have their OWN pixel dimensions. Using the
      // destination size as a SOURCE rectangle cropped large PNGs in half.
      auto fullTexture=texture->getTexture();
      int sourceWidth=0, sourceHeight=0;
      if (fullTexture && SDL_QueryTexture(fullTexture.get(), nullptr, nullptr,
                                          &sourceWidth, &sourceHeight)==0 &&
          sourceWidth>0 && sourceHeight>0) {
         TextureManager::Get().Draw(
             fullTexture, SDL_Rect{0, 0, sourceWidth, sourceHeight},
             destinationRect, texture->isHorizontalFlipped(),
             texture->isVerticalFlipped());
      }
   }''')

patch("src/TextureManager.cpp",
    '''   if (currentColor == BackgroundColor::BLUE && sbBackgroundTexture)
      SDL_RenderCopy(renderer, sbBackgroundTexture, nullptr, &backgroundRect);
   else
      SDL_RenderFillRect(renderer, &backgroundRect);''',
    '''   // Keep the v0.1.12 reference sky until the complete new HD art
   // pack has been installed. Do not regress reference-world geometry.
   if (currentColor == BackgroundColor::BLUE &&
       sb_resources::hd_enabled() && sbBackgroundTexture)
      SDL_RenderCopy(renderer, sbBackgroundTexture, nullptr, &backgroundRect);
   else
      SDL_RenderFillRect(renderer, &backgroundRect);''')

patch("include/systems/MenuSystem.h",
      "   const int maxLevel = 8;\n   const int maxSublevel = 4;",
      "   const int maxLevel = 1;\n   const int maxSublevel = 1;")
print("FIXED: texture source rectangle, flat reference sky, World 1-1 only in menu")
