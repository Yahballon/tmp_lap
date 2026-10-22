# ZQuest Engine BOOT TEST v0.0.1 (PPSA99128)

**EXPERIMENTAL / NOT PLAYABLE / HARDWARE-UNVERIFIED.** This separate app is an opt-in boot test of the **original upstream ZQuestClassic `zplayer`** cross-compiled as a native PS5 FSELF. It is not the existing v0.1.5 quest browser.

## What is real
- Original `zplayer` C++ engine and its original `main`, linked against PS5-specific SDL2/Allegro and ps5-opengl.
- Real PS5-DevBridge CRT and `_start`, real SDK import modules, converted/signed `eboot.bin`.
- Clean-room PS5 `sce_module/libc.prx`, verified with its upstream digest.
- Upstream GPL-3.0-or-later license and redistributable ZQuestClassic resources, excluding bundled quest files and NSF examples.

## What is unverified
- Console startup, display, gamepad, Quest loading, any audio, ZScript VM at runtime, saves, and exit.
- The original player has **not** been observed running on PS5 hardware.

## Testing
1. Install this **separate** PPSA99128 homebrew from TESTBUILDS. Keep PPSA99127 v0.1.5 installed.
2. Launch it on a jailbroken PS5. Report the **first screen/error**, or the point where it freezes or exits.
3. Do not expect that your `.qst` files will automatically launch; the upstream engine currently receives the normal application arguments. Quest selection is not yet wired to the native browser.
4. If needed, uninstall only PPSA99128; your `/data/ZQuestClassic/quests` files are not touched.

The first goal is a **real hardware boot signal**, not pretending a successful link equals gameplay. Source: https://github.com/ZQuestClassic/ZQuestClassic (GPLv3), PS5 platform: https://github.com/Yahballon/PS5-DevBridge.

**Separate Title-ID intentionally protects the working TESTBUILDS v0.1.5.**
