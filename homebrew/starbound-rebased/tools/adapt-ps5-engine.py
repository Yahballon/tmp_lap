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
print("Applied path, title, DualSense/SDL controller adapters without replacing upstream ECS")
