# Godot PS5 Engine Port

**Experimental Godot 4.7.2 platform source · by YAHBALLON**

**NOT PLAYABLE YET:** this is an early engine port foundation, not an installable `PPSAxxxxx.zip`.

## Goal
Run original Godot 4 games natively on jailbreak-enabled PS5 homebrew. Godot isn't emulated: its own engine must be cross-compiled for PS5 and be adapted to the console graphics, input, audio, storage and runtime interfaces.

## Implemented source
- `engine/platform/ps5/`: custom Godot platform configuration for x86_64-sie-ps5, initially *headless*.
- `godot_ps5_run_pack(const char*)`: native Godot engine startup entry point, currently not linked into an installable PS5 title.
- `pck_locator.hpp`: Godot 4 PCK header screening, deterministic directory search, symlink rejection.
- Source-only tests and standalone GDScript smoke fixture.
- No PS5 graphics, DualSense integration, audio, runtime binary or real PS5 hardware test yet.

## Intended storage layout
```text
/data/GodotPS5/games/mygame.pck
/mnt/usb0/godot/mygame.pck
/mnt/usb1/godot/mygame.pck
```
These folders become usable **only after** engine linkage, permissions and display/input support are implemented. PCK screening is not full pack validity checking.

## Verify sources
```sh
python3 homebrew/godot-ps5/scripts/check_overlay.py
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror homebrew/godot-ps5/tests/pck_locator_test.cpp -o /tmp/godot-ps5-test
/tmp/godot-ps5-test
```

## Experimental cross-build
Clone [Godot](https://github.com/godotengine/godot) at **4.7.2-stable**, and use the public PS5 SDK provisioned by [PS5-DevBridge](https://github.com/Yahballon/PS5-DevBridge):

```sh
GODOT_SOURCE=/path/to/godot PS5_PAYLOAD_SDK=/path/to/ps5-payload-sdk \
  bash homebrew/godot-ps5/scripts/build-engine.sh
```
This is an **attempt**, not a verified PS5 compile. See [ENGINE_PORTING.md](ENGINE_PORTING.md).

## Art / screenshots
Only an original vector platform placeholder (`engine/platform/ps5/logo.svg`) is present. No generated AI icon, `sce_sys/icon0.png`, or authentic gameplay screenshot exists, since there is no installed graphical app yet.

## Credits and license
Godot Engine: upstream MIT. Port overlay: YAHBALLON, MIT; [LICENSE](LICENSE). PS5-DevBridge and SDK have separate licenses.
