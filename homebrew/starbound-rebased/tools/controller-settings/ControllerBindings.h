#pragma once
#include <SDL2/SDL.h>
#include <array>
#include <string>

// Action numbers match Key. Menu controls are deliberately never remapped.
class RelayControls {
 public:
   static constexpr int axisLeft = SDL_CONTROLLER_BUTTON_MAX;
   static constexpr int axisRight = axisLeft + 1;
   static constexpr int axisUp = axisLeft + 2;
   static constexpr int axisDown = axisLeft + 3;
   static constexpr int count = axisLeft + 4;
   std::array<bool, count> now{}, previous{};
   std::array<int, 7> bindings{};
   bool sprintAlias = true;
   RelayControls() { reset(); }
   void reset() {
      bindings = {{-1, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, SDL_CONTROLLER_BUTTON_DPAD_LEFT,
                   SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_DPAD_DOWN,
                   SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_X}};
      sprintAlias = true;
   }
   void sample(const std::array<bool, count>& state) { previous = now; now = state; }
   bool pressed(int code) const { return now[code] && !previous[code]; }
   bool anyHeld() const { for (bool value : now) if (value) return true; return false; }
   bool direction(int code) const {
      if (now[code]) return true;
      if (code == SDL_CONTROLLER_BUTTON_DPAD_LEFT) return now[axisLeft];
      if (code == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) return now[axisRight];
      if (code == SDL_CONTROLLER_BUTTON_DPAD_UP) return now[axisUp];
      if (code == SDL_CONTROLLER_BUTTON_DPAD_DOWN) return now[axisDown];
      return false;
   }
   bool action(int key) const {
      if (key > 0 && key <= 6)
         return direction(bindings[key]) || (key == 5 && sprintAlias && now[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER]);
      switch (key) {
         case 7: return direction(SDL_CONTROLLER_BUTTON_DPAD_UP);
         case 8: return direction(SDL_CONTROLLER_BUTTON_DPAD_DOWN);
         case 9: return direction(SDL_CONTROLLER_BUTTON_DPAD_LEFT);
         case 10: return direction(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
         case 11: return now[SDL_CONTROLLER_BUTTON_A];
         case 12: return now[SDL_CONTROLLER_BUTTON_B];
         case 13: return now[SDL_CONTROLLER_BUTTON_START];
         default: return false;
      }
   }
   int newControl() const {
      for (int code = 0; code < count; ++code)
         if (code != SDL_CONTROLLER_BUTTON_B && code != SDL_CONTROLLER_BUTTON_START &&
             code != SDL_CONTROLLER_BUTTON_BACK && code != SDL_CONTROLLER_BUTTON_GUIDE && pressed(code)) return code;
      return -1;
   }
   void bind(int key, int code) {
      if (key < 1 || key > 6 || code < 0 || code >= count ||
          code == SDL_CONTROLLER_BUTTON_B || code == SDL_CONTROLLER_BUTTON_START ||
          code == SDL_CONTROLLER_BUTTON_BACK || code == SDL_CONTROLLER_BUTTON_GUIDE) return;
      // Stick/D-pad directions form one logical directional binding.
      if (code == axisLeft) code = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
      if (code == axisRight) code = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
      if (code == axisUp) code = SDL_CONTROLLER_BUTTON_DPAD_UP;
      if (code == axisDown) code = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
      bindings[key] = code;
      if (key == 5) sprintAlias = false;
   }
   std::string label(int key) const {
      if (key == 5 && sprintAlias) return "SQUARE / R1";
      switch (bindings[key]) {
         case SDL_CONTROLLER_BUTTON_A: return "CROSS";
         case SDL_CONTROLLER_BUTTON_X: return "SQUARE";
         case SDL_CONTROLLER_BUTTON_Y: return "TRIANGLE";
         case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return "L1";
         case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "R1";
         case SDL_CONTROLLER_BUTTON_LEFTSTICK: return "L3";
         case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return "R3";
         case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return "PAD / STICK LEFT";
         case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return "PAD / STICK RIGHT";
         case SDL_CONTROLLER_BUTTON_DPAD_UP: return "PAD / STICK UP";
         case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return "PAD / STICK DOWN";
         default: return "BUTTON " + std::to_string(bindings[key]);
      }
   }
};
