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
   // First isolate SDL core initialization from each hardware subsystem.
   // This distinguishes a broken C runtime/SDL constructor from PS5 video driver startup.
   sdl_boot_mark("TM01a before SDL_Init(0)");
   if (SDL_Init(0) != 0) {
      sdl_boot_mark("TM01b SDL_Init(0) returned error");
      return -1;
   }
   sdl_boot_mark("TM01b SDL_Init(0) successful");
   sdl_boot_mark("TM01c before SDL_INIT_TIMER");
   if (SDL_InitSubSystem(SDL_INIT_TIMER) != 0) {
      sdl_boot_mark("TM01d timer init failed"); return -1;
   }
   sdl_boot_mark("TM01d timer init successful");
   sdl_boot_mark("TM01e before SDL_INIT_EVENTS");
   if (SDL_InitSubSystem(SDL_INIT_EVENTS) != 0) {
      sdl_boot_mark("TM01f event init failed"); return -1;
   }
   sdl_boot_mark("TM01f event init successful");
   sdl_boot_mark("TM01g before SDL_INIT_VIDEO");
   if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
      sdl_boot_mark("TM02 video init returned error");''')
patch("src/TextureManager.cpp",
      '   if (IMG_Init(IMG_INIT_PNG) < 0) {',
      '''   sdl_boot_mark("TM02 SDL video initialization successful");
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


# Preserve exact stage trace through the first two frames. A crash after
# Core::run was previously indistinguishable from an SDL event pump crash.
patch("src/Core.cpp",
      '#include "Core.h"',
      '''#include "Core.h"
#ifdef __FreeBSD__
#include <cstdio>
#include <sys/stat.h>
static void core_frame_trace(const char* msg) {
    mkdir("/data/starbound-rebased", 0755);
    if (FILE* f=fopen("/data/starbound-rebased/boot.log", "a")) {
        fprintf(f, "%s\\n", msg); fflush(f); fclose(f);
    }
}
#else
static void core_frame_trace(const char*) {}
#endif''')
patch("src/Core.cpp",
      '''   Uint64 startTicks = SDL_GetTicks64();
   game.handleInput();
   game.update();
   CommandScheduler::getInstance().run();
   limitFPS(startTicks);
   std::cout << std::flush;''',
      '''   static unsigned frame_number=0;
   const bool trace=frame_number<2;
   if (trace) core_frame_trace("LOOP01 before SDL_GetTicks64");
   Uint64 startTicks = SDL_GetTicks64();
   if (trace) core_frame_trace("LOOP02 before game.handleInput");
   game.handleInput();
   if (trace) core_frame_trace("LOOP03 after game.handleInput");
   game.update();
   if (trace) core_frame_trace("LOOP04 after game.update");
   CommandScheduler::getInstance().run();
   if (trace) core_frame_trace("LOOP05 after scheduler");
   limitFPS(startTicks);
   if (trace) core_frame_trace("LOOP06 first frame complete");
   ++frame_number;
   std::cout << std::flush;''')

# The first scene uses a font and logo, so guard missing conversion results.
patch("src/systems/RenderSystem.cpp",
      '''      SDL_Surface* textSurface =
          TTF_RenderText_Blended(normalFont.get(), textComponent->text.c_str(), color);''',
      '''      if (!normalFont) return;
      SDL_Surface* textSurface =
          TTF_RenderText_Blended(normalFont.get(), textComponent->text.c_str(), color);
      if (!textSurface) return;''')
patch("src/systems/RenderSystem.cpp",
      '''      SDL_Texture* texture =
          SDL_CreateTextureFromSurface(TextureManager::Get().getRenderer(), textSurface);''',
      '''      SDL_Texture* texture =
          SDL_CreateTextureFromSurface(TextureManager::Get().getRenderer(), textSurface);
      if (!texture) { SDL_FreeSurface(textSurface); return; }''')

# Tracing GPU/software present distinguishes an SDL input crash from a
# window framebuffer / sceVideoOutSubmitFlip failure.
patch("src/systems/RenderSystem.cpp",
      '#include "systems/RenderSystem.h"',
      '''#include "systems/RenderSystem.h"
#ifdef __FreeBSD__
#include <cstdio>
#include <sys/stat.h>
static void ps5_render_trace(const char* msg) {
    mkdir("/data/starbound-rebased", 0755);
    if (FILE* f=fopen("/data/starbound-rebased/boot.log", "a")) {
        fprintf(f, "%s\\n", msg); fflush(f); fclose(f);
    }
}
#else
static void ps5_render_trace(const char*) {}
#endif''')
patch("src/systems/RenderSystem.cpp",
      '''void RenderSystem::tick(World* world) {
   TextureManager::Get().Clear();''',
      '''void RenderSystem::tick(World* world) {
   static int traced_frames=0;
   if (traced_frames<2) ps5_render_trace("RENDER01 begin RenderSystem::tick");
   TextureManager::Get().Clear();
   if (traced_frames<2) ps5_render_trace("RENDER02 cleared framebuffer");''')
patch("src/systems/RenderSystem.cpp",
      '''   TextureManager::Get().Display();
}''',
      '''   if (traced_frames<2) ps5_render_trace("RENDER03 before SDL_RenderPresent");
   TextureManager::Get().Display();
   if (traced_frames<2) ps5_render_trace("RENDER04 after SDL_RenderPresent");
   ++traced_frames;
}''')

print("Applied path, title, DualSense/SDL controller adapters without replacing upstream ECS")
