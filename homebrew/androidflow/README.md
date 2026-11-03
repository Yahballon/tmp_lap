# AndroidFlow

**APK Coverflow for PS5 · by YAHBALLON**

![AndroidFlow illustrated concept logo](assets/logo.svg)

> **Experimental source prototype. Not a PS5 TESTBUILD release.** The source implements a coverflow picker and hooks into Hydroid, but needs a full PS5 native build and console validation.

AndroidFlow uses [Hydroid](https://github.com/ZiZc3/Hydroid)'s ARM64 Android game runtime, built-in APK metadata/icon extraction and per-game saves. The visual browsing style is adapted from the GPL GBAXY coverflow layout in [PS5GBAX](https://github.com/Yahballon/PS5GBAX). All game assets come from user-owned Android packages; none are distributed with AndroidFlow.

## What is implemented in source

- Native full-screen 1920x1080 animated card coverflow with a larger focused card and angled neighbors.
- Detect .apk, .xapk, .apks and .apkm packages; display game names and embedded app icons using Hydroid's parser and cache.
- A single title can select among multiple packages and feed the chosen path to Hydroid's existing run_apk function.
- D-pad/left-stick browse, L1/R1 jump by five, Cross start, Triangle library rescan, Circle exit.
- Distinct experimental Title ID PPSA99920 with its own persistent library/cache root.

The current library scans up to 64 files across:

    /data/homebrew/PPSA99920/game/
    /mnt/usb0/Android/
    /mnt/usb1/Android/
    /data/AndroidFlow/apks/

USB permissions and paths depend on your jailbreak and loader. Scanning is synchronous in this version. Returning from an Android game to choose a different one requires restarting the launcher.

## Compatibility reality

Hydroid is not a full Android operating system. At present it targets selected 64-bit ARM Android games whose engine and API calls are implemented. That includes some Unity, Cocos2d-x, Godot, Unreal and NativeActivity games. Many other Android apps, especially pure Java/Kotlin or 32-bit-only packages, will not work. Google Play services and arbitrary APK installation aren't provided. Launching a given game is unverified until tested on PS5.

## Build

See upstream [Hydroid BUILD.md](https://github.com/ZiZc3/Hydroid/blob/main/BUILD.md) first. This requires the Android NDK r27c, PS5_Vulkan/native payload toolchain, ps5-opengl, Dynarmic, FFmpeg for PS5 and other build dependencies. They are **not** vendored in this repository.

From an existing local clone of Yahballon/tmp_lap:

    git clone https://github.com/ZiZc3/Hydroid.git /tmp/hydroid-androidflow
    git -C /tmp/hydroid-androidflow checkout --detach d8371afd75e3da6ab888d467d1e6bb00c32d2beb
    python3 -m pip install Pillow
    python3 homebrew/androidflow/tools/apply_overlay.py --source /tmp/hydroid-androidflow
    cd /tmp/hydroid-androidflow
    bash third_party/fetch-boost.sh
    bash title/build-title.sh

The **expected** native output after a successful build is dist/PPSA99920, with eboot.bin, sce_sys/param.json, sce_sys/icon0.png, sce_module/libc.prx, fonts, native dependencies and androidflow.flag. A ZIP of only this source folder is **not** installable. Check output and required dependencies before creating a test release.

The [overlay validation workflow](../../.github/workflows/check-androidflow.yml) passed in run 41590133362 (host math and pinned source patch). A separate [native PS5 build workflow](../../.github/workflows/build-androidflow.yml) now attempts the full pinned Hydroid/RADV/Dynarmic/NDK/FFmpeg toolchain, independently validates the signed PPSA99920 installation ZIP and publishes an immutable release only after a passing main-branch build. The native build has not yet been observed passing. The generated central version.json is intentionally left untouched until there is a verified installable ZIP in a real GitHub Release.

## Visuals & attribution

The standalone logo here is an original, **programmatically drawn** illustration, not an AI-generated icon and not a screen capture. The icon script (assets/generate_icon.py) outputs the actual 512x512 sce_sys/icon0.png during the overlay. A genuine screenshot is pending PS5 or host rendering.

- Hydroid source and runtime: ZiZc3, GPL-3.0, with its original licensing and required dependency notices.
- GBAXY/PS5GBAX coverflow geometry: GPL-3.0-or-later.
- AndroidFlow integration: YAHBALLON, GPL-3.0-or-later.

**Development handoff:** [MILESTONES.md](MILESTONES.md).
