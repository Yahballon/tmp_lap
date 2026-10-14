#include "Input.h"

Input Input::instance;

void Input::initDefault() {
   keyBindings = {{Key::NONE, SDL_SCANCODE_UNKNOWN}, {Key::RIGHT, SDL_SCANCODE_D},
      {Key::LEFT, SDL_SCANCODE_A}, {Key::JUMP, SDL_SCANCODE_SPACE},
      {Key::DUCK, SDL_SCANCODE_S}, {Key::SPRINT, SDL_SCANCODE_LSHIFT},
      {Key::FIREBALL, SDL_SCANCODE_Q}, {Key::MENU_UP, SDL_SCANCODE_UP},
      {Key::MENU_DOWN, SDL_SCANCODE_DOWN}, {Key::MENU_LEFT, SDL_SCANCODE_LEFT},
      {Key::MENU_RIGHT, SDL_SCANCODE_RIGHT}, {Key::MENU_ACCEPT, SDL_SCANCODE_RETURN},
      {Key::MENU_ESCAPE, SDL_SCANCODE_ESCAPE}, {Key::PAUSE, SDL_SCANCODE_ESCAPE}};
   for (auto key : keyBindings) { keysPressed[key.first] = false; keysHeld[key.first] = false; }
}

void Input::set(Key action, SDL_Scancode code) {
   keyBindings[action] = code;
   // Retain current edge history: the captured button must not confirm again.
}

void Input::update(const Uint8* keyboard) {
   currentRawKeys.clear();
   newKey = SDL_SCANCODE_UNKNOWN;
   keyboardHeld = false;
   for (int code = 1; code < SDL_NUM_SCANCODES; ++code) {
      if (keyboard[code]) {
         keyboardHeld = true;
         if (!previousKeyboard[code] && newKey == SDL_SCANCODE_UNKNOWN)
            newKey = static_cast<SDL_Scancode>(code);
      }
      previousKeyboard[code] = keyboard[code];
   }
   if (!(SDL_WasInit(SDL_INIT_GAMECONTROLLER) & SDL_INIT_GAMECONTROLLER))
      SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK);
   if (pad && !SDL_GameControllerGetAttached(pad)) { SDL_GameControllerClose(pad); pad = nullptr; }
   if (!pad) {
      for (int index = 0; index < SDL_NumJoysticks(); ++index)
         if (SDL_IsGameController(index)) { pad = SDL_GameControllerOpen(index); if (pad) break; }
   }
   SDL_GameControllerUpdate();
   std::array<bool, RelayControls::count> state{};
   if (pad) {
      for (int button = 0; button < SDL_CONTROLLER_BUTTON_MAX; ++button)
         state[button] = SDL_GameControllerGetButton(pad, static_cast<SDL_GameControllerButton>(button)) != 0;
      const int x = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX);
      const int y = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY);
      state[RelayControls::axisLeft] = x < -13000;
      state[RelayControls::axisRight] = x > 13000;
      state[RelayControls::axisUp] = y < -13000;
      state[RelayControls::axisDown] = y > 13000;
   }
   controls.sample(state);
   for (auto key : keyBindings) {
      bool down = keyboard[key.second] || controls.action(static_cast<int>(key.first));
      keysHeld[key.first] = down && keysPressed[key.first];
      keysPressed[key.first] = down;
   }
}
SDL_Scancode Input::getBoundKey(Key key) { return keyBindings.at(key); }
bool Input::getRawKey(Key key) { return keysPressed.at(key); }
bool Input::getKeyPressed(Key key) { return keysPressed.at(key) && !keysHeld.at(key); }
bool Input::getKeyHeld(Key key) { return keysHeld.at(key); }
std::vector<SDL_Scancode>& Input::getCurrentRawKeys() { return currentRawKeys; }
