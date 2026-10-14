#!/usr/bin/env python3
"""Exercise real Input and Options tick/handleInput using a virtual SDL pad."""
from pathlib import Path
import sys
engine, output = map(Path, sys.argv[1:3])
source = (engine / 'src/systems/OptionsSystem.cpp').read_text()
assert 'SDL_WaitEvent' not in source
assert 'PRESS ESC TO CANCEL' not in source
tick = source[source.index('void OptionsSystem::tick('):source.index('void OptionsSystem::hideKeySelectEntities(')]
output.write_text(r'''
#include "Input.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
constexpr int SCALED_CUBE_SIZE = 32;
struct PositionComponent { struct Point { float x=0, y=0; } position, scale; };
struct TextComponent { std::string text; void destroyTexture() {} };
struct Entity {
   PositionComponent position;
   TextComponent text;
   template<class T> T* getComponent();
};
template<> PositionComponent* Entity::getComponent<PositionComponent>() { return &position; }
template<> TextComponent* Entity::getComponent<TextComponent>() { return &text; }
struct World {};
struct OptionsSystem {
   Entity storage[10];
   Entity* cursor=&storage[0];
   Entity* keyUnderline=&storage[1];
   Entity* infoTextEnter=&storage[2];
   const int totalOptions=6;
   int currentSelectedOption=0;
   Key currentWaitingKey=Key::NONE;
   bool finished=false, visible=false;
   std::unordered_map<int,Key> optionsKeyMap={{0,Key::LEFT},{1,Key::RIGHT},{2,Key::JUMP},
      {3,Key::DUCK},{4,Key::SPRINT},{5,Key::FIREBALL}};
   std::unordered_map<Key,Entity*> keyEntityMap;
   OptionsSystem() { for (auto entry : optionsKeyMap) keyEntityMap[entry.second]=&storage[entry.first+3]; }
   void tick(World*);
   void handleInput();
   void hideKeySelectEntities() { visible=false; }
   void showKeySelectEntities() { visible=true; }
   std::string getKeyString(Key) { return "ACTION"; }
   std::string getKeybindString(Key key) {
      return Input::Get().usingController() ? Input::Get().controller().label(static_cast<int>(key)) :
         SDL_GetScancodeName(Input::Get().getBoundKey(key));
   }
};
''' + tick + r'''
int main() {
   assert(SDL_Init(SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) == 0);
   int device=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,6,SDL_CONTROLLER_BUTTON_MAX,0);
   assert(device>=0 && SDL_IsGameController(device));
   SDL_Joystick* joystick=SDL_JoystickOpen(device);
   Input& input=Input::Get();
   std::array<Uint8,SDL_NUM_SCANCODES> keyboard{};
   auto frame=[&](OptionsSystem* options=nullptr) {
      SDL_PumpEvents(); SDL_JoystickUpdate(); input.update(keyboard.data());
      if (options) { options->handleInput(); options->tick(nullptr); }
   };
   auto button=[&](int code,bool held) { assert(SDL_JoystickSetVirtualButton(joystick,code,held)==0); };
   frame();
   button(SDL_CONTROLLER_BUTTON_A,true); frame();
   assert(input.getKeyPressed(Key::JUMP) && input.getKeyPressed(Key::MENU_ACCEPT));
   frame(); assert(!input.getKeyPressed(Key::JUMP) && input.getKeyHeld(Key::JUMP));
   button(SDL_CONTROLLER_BUTTON_A,false); button(SDL_CONTROLLER_BUTTON_X,true); frame();
   assert(input.getRawKey(Key::SPRINT) && input.getRawKey(Key::FIREBALL));
   button(SDL_CONTROLLER_BUTTON_X,false); button(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,true); frame();
   assert(input.getRawKey(Key::SPRINT) && !input.getRawKey(Key::FIREBALL));
   button(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,false);
   button(SDL_CONTROLLER_BUTTON_B,true); frame();
   assert(input.getKeyPressed(Key::MENU_ESCAPE) && !input.getRawKey(Key::PAUSE));
   button(SDL_CONTROLLER_BUTTON_B,false); button(SDL_CONTROLLER_BUTTON_START,true); frame();
   assert(input.getKeyPressed(Key::PAUSE) && !input.getRawKey(Key::MENU_ESCAPE));
   button(SDL_CONTROLLER_BUTTON_START,false); frame();
   SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_LEFTX,20000); frame();
   assert(input.getRawKey(Key::RIGHT) && input.getRawKey(Key::MENU_RIGHT));
   SDL_JoystickSetVirtualAxis(joystick,SDL_CONTROLLER_AXIS_LEFTX,0); frame();
   OptionsSystem options; options.currentSelectedOption=2;
   button(SDL_CONTROLLER_BUTTON_A,true); frame(&options);
   assert(options.visible && options.currentWaitingKey==Key::JUMP);
   for (int i=0;i<20;++i) frame(&options);
   assert(options.currentWaitingKey==Key::JUMP); // Opening Cross cannot bind itself.
   button(SDL_CONTROLLER_BUTTON_B,true); frame(&options);
   assert(!options.visible && !options.finished && options.currentWaitingKey==Key::NONE);
   assert(input.controller().label(static_cast<int>(Key::JUMP))=="CROSS");
   button(SDL_CONTROLLER_BUTTON_A,false); button(SDL_CONTROLLER_BUTTON_B,false); frame(&options);
   button(SDL_CONTROLLER_BUTTON_A,true); frame(&options);
   button(SDL_CONTROLLER_BUTTON_A,false); frame(&options);
   button(SDL_CONTROLLER_BUTTON_LEFTSHOULDER,true); frame(&options);
   assert(options.currentWaitingKey==Key::NONE && !options.finished);
   assert(input.controller().label(static_cast<int>(Key::JUMP))=="L1");
   frame(); assert(input.getRawKey(Key::JUMP));
   button(SDL_CONTROLLER_BUTTON_LEFTSHOULDER,false); button(SDL_CONTROLLER_BUTTON_A,true); frame();
   assert(!input.getRawKey(Key::JUMP) && input.getRawKey(Key::MENU_ACCEPT));
   button(SDL_CONTROLLER_BUTTON_A,false); frame();
   button(SDL_CONTROLLER_BUTTON_Y,true); frame(&options);
   assert(input.controller().label(static_cast<int>(Key::JUMP))=="CROSS");
   button(SDL_CONTROLLER_BUTTON_Y,false); frame();
   button(SDL_CONTROLLER_BUTTON_B,true); frame(&options); assert(options.finished);
   button(SDL_CONTROLLER_BUTTON_B,false); frame();
   SDL_JoystickClose(joystick); assert(SDL_JoystickDetachVirtual(device)==0); frame();
   assert(!input.getRawKey(Key::MENU_ESCAPE) && !input.getRawKey(Key::JUMP));
   OptionsSystem desktop; desktop.currentSelectedOption=2;
   keyboard[SDL_SCANCODE_RETURN]=1; frame(&desktop);
   assert(desktop.currentWaitingKey==Key::JUMP);
   frame(&desktop); assert(desktop.currentWaitingKey==Key::JUMP);
   keyboard[SDL_SCANCODE_ESCAPE]=1; frame(&desktop);
   assert(desktop.currentWaitingKey==Key::NONE && !desktop.finished);
   keyboard.fill(0); frame();
   keyboard[SDL_SCANCODE_RETURN]=1; frame(&desktop);
   keyboard.fill(0); frame(&desktop);
   keyboard[SDL_SCANCODE_Z]=1; frame(&desktop);
   assert(input.getBoundKey(Key::JUMP)==SDL_SCANCODE_Z);
   frame(); assert(input.getRawKey(Key::JUMP));
   std::cout << "PASS: real controller mappings, edges, stick menus, cancel, release gate, remap, reset, disconnect and keyboard capture\n";
   SDL_Quit();
}
''')
print('Generated actual Input/Options controller regression harness')
