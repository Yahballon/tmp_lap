#!/usr/bin/env python3
"""Minimal console platform adaptation on a pinned Gold872 C++ checkout.
The ECS, physics, game scenes and original control logic remain intact.
"""
from pathlib import Path
import sys
root=Path(sys.argv[1]) if len(sys.argv)>1 else Path("build/gold872-engine")
def patch(relative,old,new):
    path=root/relative
    content=path.read_text()
    if old not in content:raise RuntimeError(f"Expected upstream source missing: {relative}")
    path.write_text(content.replace(old,new,1))
patch("src/main.cpp",
      '#define SDL_MAIN_HANDLED',
      '#define SDL_MAIN_HANDLED\n#include <unistd.h>')
patch("src/main.cpp",
      'int main(int argc, char** argv) {',
      '''int main(int argc, char** argv) {
#ifdef __FreeBSD__
   // ELF loader does not guarantee the game's working directory.
   if (access("res/sprites/characters/PlayerSpriteSheet.png", F_OK) != 0) {
      if (chdir("/app0/assets") != 0 && chdir("/data/homebrew/PPSA99642/assets") != 0)
         chdir("/mnt/usb0/STARBOUND_REBASED");
   }
#endif''')
patch("src/TextureManager.cpp",
      '"Super Mario Bros", SDL_WINDOWPOS_CENTERED',
      '"Starbound Rebased", SDL_WINDOWPOS_CENTERED')
patch("src/Input.cpp",
      '''   for (auto key : keyBindings) {
      // If key pressed
      if (keystates[key.second]) {''',
      '''#ifdef __FreeBSD__
   static SDL_GameController* pad=nullptr;
   static bool gamepadInit=false;
   if (!gamepadInit) {
      SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK);
      gamepadInit=true;
   }
   if (!pad && SDL_NumJoysticks()>0 && SDL_IsGameController(0))
      pad=SDL_GameControllerOpen(0);
   SDL_GameControllerUpdate();
#endif
   for (auto key : keyBindings) {
      bool down=(keystates[key.second]!=0);
#ifdef __FreeBSD__
      if (pad) {
         auto btn=[&](SDL_GameControllerButton b){return SDL_GameControllerGetButton(pad,b)!=0;};
         auto axis=[&](SDL_GameControllerAxis a){return SDL_GameControllerGetAxis(pad,a);};
         switch(key.first){
            case Key::LEFT: down|=btn(SDL_CONTROLLER_BUTTON_DPAD_LEFT)||axis(SDL_CONTROLLER_AXIS_LEFTX)<-13000;break;
            case Key::RIGHT: down|=btn(SDL_CONTROLLER_BUTTON_DPAD_RIGHT)||axis(SDL_CONTROLLER_AXIS_LEFTX)>13000;break;
            case Key::DUCK: down|=btn(SDL_CONTROLLER_BUTTON_DPAD_DOWN);break;
            case Key::JUMP: down|=btn(SDL_CONTROLLER_BUTTON_A);break;
            case Key::SPRINT: down|=btn(SDL_CONTROLLER_BUTTON_X)||btn(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);break;
            case Key::FIREBALL: down|=btn(SDL_CONTROLLER_BUTTON_X);break;
            case Key::MENU_UP: down|=btn(SDL_CONTROLLER_BUTTON_DPAD_UP);break;
            case Key::MENU_DOWN: down|=btn(SDL_CONTROLLER_BUTTON_DPAD_DOWN);break;
            case Key::MENU_LEFT: down|=btn(SDL_CONTROLLER_BUTTON_DPAD_LEFT);break;
            case Key::MENU_RIGHT: down|=btn(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);break;
            case Key::MENU_ACCEPT: down|=btn(SDL_CONTROLLER_BUTTON_A);break;
            case Key::MENU_ESCAPE:
            case Key::PAUSE: down|=btn(SDL_CONTROLLER_BUTTON_START);break;
            default:break;
         }
      }
#endif
      // If key pressed
      if (down) {''')

# Diagnostic checkpoints allow us to locate the boot crash even before SDL
# creates a window. Use stdio instead of C++ streams in early diagnostics.
patch("src/main.cpp",
      '#include "Core.h"',
      '''#include "Core.h"
#include <cstdio>
#include <sys/stat.h>
static void boot_trace(const char* point) {
#ifdef __FreeBSD__
   mkdir("/data/starbound-rebased",0755);
   FILE* f=fopen("/data/starbound-rebased/boot.log","a");
   if (f) { fprintf(f,"%s\\n",point);fflush(f);fclose(f); }
#endif
}
''')
patch("src/main.cpp",
      '   Core core;',
      '''   boot_trace("01: main entered");
   Core core;
   boot_trace("02: core constructed");''')
patch("src/main.cpp",
      '   if (core.init() != 0) {',
      '''   boot_trace("03: calling Core::init");
   if (core.init() != 0) {
      boot_trace("ERROR: Core::init failed");''')
patch("src/main.cpp",
      '   core.run();',
      '''   boot_trace("04: entering Core::run");
   core.run();''')
patch("src/Core.cpp",
      '   if (TextureManager::Get().Init() != 0) {',
      '''#ifdef __FreeBSD__
   SDL_Log("Starbound: before SDL video initialization");
#endif
   if (TextureManager::Get().Init() != 0) {''')
patch("src/TextureManager.cpp",
      '   SDL_SetWindowIcon(window, iconSurface);',
      '''   if (iconSurface) SDL_SetWindowIcon(window, iconSurface);''')
patch("src/TextureManager.cpp",
      '   SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, tempSurface);',
      '''   SDL_Texture* texture = tempSurface ? SDL_CreateTextureFromSurface(renderer, tempSurface) : nullptr;''')
patch("src/TextureManager.cpp",
      '   if (blueTransparent) {',
      '''   if (!surface) {
      SDL_Log("Missing texture: %s: %s",path,IMG_GetError());
      return {};
   }
   if (blueTransparent) {''')
patch("src/TextureManager.cpp",
      '   SDL_Surface* textSurface = TTF_RenderText_Blended(font.get(), text, color);',
      '''   if (!font || !text || !renderer) return;
   SDL_Surface* textSurface = TTF_RenderText_Blended(font.get(), text, color);
   if (!textSurface) return;''')
# SDL2 audio is optional: do not abort the game when console audio driver is missing.
patch("src/Core.cpp",
      '   if (SoundManager::Get().Init() != 0) {',
      '''   if (SoundManager::Get().Init() != 0) {''')
patch("src/Core.cpp",
      '      return -1;\n   }\n   srand(time(NULL));',
      '''      SDL_Log("Audio unavailable: continuing without audio");
   }
   srand(time(NULL));''')


# Persist individual SDL startup checkpoints before touching each subsystem.
patch("src/TextureManager.cpp",
      'TextureManager TextureManager::instance;',
      '''TextureManager TextureManager::instance;
#ifdef __FreeBSD__
#include <cstdio>
#include <sys/stat.h>
static void sdl_boot_mark(const char* text) {
    mkdir("/data/starbound-rebased",0755);
    if(FILE* f=fopen("/data/starbound-rebased/boot.log","a")) {
        fprintf(f,"%s\\n",text); fflush(f); fclose(f);
    }
}
#else
static void sdl_boot_mark(const char*) {}
#endif''')
patch("src/TextureManager.cpp",
      '   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {',
      '''   sdl_boot_mark("TM01 before SDL_Init");
   // Initialize video separately: PS5 SDL2 audio drivers may fail independently.
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
      sdl_boot_mark("TM02 SDL_Init returned error");''')
patch("src/TextureManager.cpp",
      '   if (IMG_Init(IMG_INIT_PNG) < 0) {',
      '''   sdl_boot_mark("TM02 SDL_Init success");
   sdl_boot_mark("TM03 before IMG_Init");
   if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) {
      sdl_boot_mark("TM04 IMG_Init failed");''')
patch("src/TextureManager.cpp",
      '   if (TTF_Init() != 0) {',
      '''   sdl_boot_mark("TM04 IMG_Init success");
   sdl_boot_mark("TM05 before TTF_Init");
   if (TTF_Init() != 0) {
      sdl_boot_mark("TM06 TTF_Init failed");''')
patch("src/TextureManager.cpp",
      '   window = SDL_CreateWindow("Starbound Rebased",',
      '''   sdl_boot_mark("TM06 TTF_Init success");
   sdl_boot_mark("TM07 before SDL_CreateWindow");
   window = SDL_CreateWindow("Starbound Rebased",''')
patch("src/TextureManager.cpp",
      '   SDL_Surface* iconSurface = IMG_Load("res/sprites/icons/windowicon.png");',
      '''   sdl_boot_mark("TM08 SDL_CreateWindow success");
   SDL_Surface* iconSurface = IMG_Load("res/sprites/icons/windowicon.png");''')
patch("src/TextureManager.cpp",
      '   renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);',
      '''   sdl_boot_mark("TM09 before SDL_CreateRenderer");
   renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
   if (!renderer) {
      sdl_boot_mark("TM10 hardware renderer unavailable, trying software");
      renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
   }''')
patch("src/TextureManager.cpp",
      '   if (SDL_SetRenderDrawColor(renderer, 92, 148, 252, 255) != 0) {',
      '''   sdl_boot_mark("TM11 SDL renderer success");
   if (SDL_SetRenderDrawColor(renderer, 92, 148, 252, 255) != 0) {''')
patch("src/TextureManager.cpp",
      '   return 0;\n}\n\nint TextureManager::Quit()',
      '''   sdl_boot_mark("TM12 TextureManager init complete");
   return 0;
}

int TextureManager::Quit()''')

print("Applied path, title, DualSense/SDL controller adapters without replacing upstream ECS")
