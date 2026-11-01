# ScummVM PS5

**Classic adventure engines on PS5 · by YAHBALLON**

Experimental v0.1.0 · Candidate title ID **PPSA99053** (not an official reservation). This is a **dedicated PS5 RetroArch/Libretro ScummVM build**, not a native standalone ScummVM GUI. It includes the ScummVM core only, retains RetroArch's game browser and PS5-native controller/audio/video backend, and is intended to start in the ScummVM core menu. Upstream hardware evidence establishes the core is viable, **not** that this dedicated build works yet.

## Credits and upstream

- [ScummVM](https://github.com/scummvm/scummvm) (GPL-3.0-or-later; retained upstream notices)
- [Mihawk's PS5 ScummVM](https://github.com/mihawk-99/PS5_ScummVM) at `967fd9757170b14c28590837186b31cc7d5cc46c`
- [Mihawk's PS5 RetroArch](https://github.com/mihawk-99/PS5_RetroArch) at `7268510b12fe7461342e0cc4497c87a0910c1055`

The original generated adventure-compass artwork is stored as `assets/icon-source.webp`; `scripts/prepare.py` converts it into the actual installer's `sce_sys/icon0.png`. No Sony marks, commercial game art, game data, BIOS, or copyrighted game collections are bundled. The icon is original artwork created for this port.

## Build and install

The monorepo workflow is [build-scummvm-ps5.yml](../../.github/workflows/build-scummvm-ps5.yml). It pins both upstream commits, installs the dependencies used by Mihawk's PS5 RetroArch, builds its existing PS5 Vulkan stack and one ScummVM libretro core, then signs/repackages the real PS5 title to `dist/PPSA99053.zip`. This native build is expensive and has **not yet been verified**.

```bash
python3 homebrew/scummvm-ps5/scripts/prepare.py --source /path/to/PS5_RetroArch
# In the patched source checkout, after installing PS5 toolchain and RADV dependencies:
PS5_VULKAN_DIR=/path/to/PS5_Vulkan bash tools/build-title.sh
python3 /path/to/tmp_lap/homebrew/scummvm-ps5/scripts/package.py /path/to/PS5_RetroArch/dist
```

The expected ZIP root is `PPSA99053/`, for installation under the homebrew launcher as `/data/homebrew/PPSA99053/`. Use a compatible jailbroken PS5; PS5 hardware validation is outstanding. Do not overwrite another app with a colliding Title ID. The launcher is a restricted RetroArch build and displays its own menu; the embedded engine UI must still be tested.

## User data and controls

Add your own legally obtained ScummVM game data through RetroArch's Load Content / ScummVM workflow. ScummVM's `.scummvm` game pointers and no-game launch capability are provided by its libretro core. Place data in a directory that your current jailbreak and storage permissions actually expose; do not assume USB mounts are accessible. **Back up saves before replacing any experimental build.** Existing RetroArch menu/gamepad bindings and the ScummVM in-game virtual mouse vary with the selected core options. The native input driver is PS5; actual touchpad, mouse emulation, saves, sound and USB access are *not yet hardware-verified* in this title.

## Development status

See [MILESTONES.md](MILESTONES.md) for test evidence, remaining work and reproducible handoff. **No confirmed PS5-native binary, release, or TESTBUILDS catalog entry yet.** `version.json` is maintained by GitHub Actions and must not be edited manually. SCREENSHOT: NOT CAPTURED (no device session available).
