#!/usr/bin/env python3
"""Install nonblocking keyboard/DualSense rebinding in the pinned engine."""
from pathlib import Path
import sys

root = Path(sys.argv[1])
assets = Path(__file__).parent / 'controller-settings'
for name in ('ControllerBindings.h',):
    (root / 'include' / name).write_text((assets / name).read_text())
(root / 'src/Input.cpp').write_text((assets / 'Input.cpp').read_text())
path = root / 'include/Input.h'
text = path.read_text().replace('#include <unordered_map>', '#include "ControllerBindings.h"\n#include <array>\n#include <unordered_map>')
text = text.replace('   SDL_Scancode getBoundKey(Key action);', '''   SDL_Scancode getBoundKey(Key action);
   RelayControls& controller() { return controls; }
   bool usingController() const {
#ifdef __FreeBSD__
      return true;
#else
      return pad != nullptr;
#endif
   }
   SDL_Scancode newlyPressedKey() const { return newKey; }
   bool anyKeyboardHeld() const { return keyboardHeld; }
   void beginCapture() { captureController = usingController(); captureArmed = false; }
   bool captureReady() {
      if (!captureArmed) {
         captureArmed = captureController ? !controls.anyHeld() : !keyboardHeld;
         return false;
      }
      return true;
   }
   bool capturesController() const { return captureController; }''')
text = text.replace('   static Input instance;', '''   RelayControls controls;
   SDL_GameController* pad = nullptr;
   std::array<Uint8, SDL_NUM_SCANCODES> previousKeyboard{};
   SDL_Scancode newKey = SDL_SCANCODE_UNKNOWN;
   bool keyboardHeld = false, captureArmed = false, captureController = false;
   static Input instance;''')
path.write_text(text)
path = root / 'src/systems/OptionsSystem.cpp'
text = path.read_text()
text = text.replace('   SDL_Keycode keycode =', '''   if (Input::Get().usingController())
      return Input::Get().controller().label(static_cast<int>(key));
   SDL_Keycode keycode =''')
for label in ('LEFT', 'RIGHT', 'JUMP', 'DUCK', 'SPRINT', 'FIREBALL'):
    text = text.replace(f'"{label} KEY:"', f'"{label}:"')
text = text.replace('Vector2f(14,', 'Vector2f(12,')
for key in ('LEFT', 'RIGHT', 'JUMP', 'DUCK', 'SPRINT', 'FIREBALL'):
    text = text.replace(f'getKeybindString(Key::{key}), 16', f'getKeybindString(Key::{key}), 12')
text = text.replace('"GO BACK", 16', 'Input::Get().usingController() ? "CIRCLE: BACK / TRIANGLE: RESET" : "GO BACK", 12')
text = text.replace('"PRESS ESC TO CANCEL", 16', 'Input::Get().usingController() ? "CIRCLE: CANCEL" : "ESC: CANCEL", 12')
text = text.replace('"PRESS KEY FOR ", 16', '"SELECT CONTROL", 12')
begin = text.index('   SDL_Scancode pressedKey =', text.index('void OptionsSystem::tick('))
end = text.index('   Entity* keyText =', begin)
text = text[:begin] + '''   Input& input = Input::Get();
   // Cancel wins even before the opening Cross/Enter has been released.
   if (input.getKeyPressed(Key::MENU_ESCAPE) || input.getKeyPressed(Key::PAUSE)) {
      hideKeySelectEntities();
      currentWaitingKey = Key::NONE;
      return;
   }
   if (!input.captureReady()) return;
   if (input.capturesController()) {
      int control = input.controller().newControl();
      if (control < 0) return;
      input.controller().bind(static_cast<int>(currentWaitingKey), control);
   } else {
      SDL_Scancode control = input.newlyPressedKey();
      if (control == SDL_SCANCODE_UNKNOWN) return;
      input.set(currentWaitingKey, control);
   }

''' + text[end:]
text = text.replace('   if (input.getKeyPressed(Key::MENU_DOWN)) {', '''   if (input.usingController() && input.controller().pressed(SDL_CONTROLLER_BUTTON_Y)) {
      input.controller().reset();
      for (auto entry : keyEntityMap) {
         auto* label = entry.second->getComponent<TextComponent>();
         label->destroyTexture();
         label->text = getKeybindString(entry.first);
      }
   }
   if (input.getKeyPressed(Key::MENU_DOWN)) {''')
text = text.replace('         currentWaitingKey = optionsKeyMap.at(currentSelectedOption);', '         currentWaitingKey = optionsKeyMap.at(currentSelectedOption);\n         input.beginCapture();')
text = text.replace('"PRESS KEY FOR " + getKeyString(currentWaitingKey)', '(input.capturesController() ? "CONTROL FOR " : "KEY FOR ") + getKeyString(currentWaitingKey)')
# Back/cancel must not start a capture if both buttons arrive together.
text = text.replace('   if (input.getKeyPressed(Key::MENU_ACCEPT)) {', '   if (input.getKeyPressed(Key::MENU_ACCEPT) && !input.getKeyPressed(Key::MENU_ESCAPE)) {')
path.write_text(text)
print('Installed DualSense labels, actual action rebinding, release gate and nonblocking cancel')
