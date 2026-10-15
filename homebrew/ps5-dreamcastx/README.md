# PS5DreamcastX

**Dreamcast emulation on PS5 with a PS5GBAX-inspired animated cover-flow shelf.**

Version **0.1.0** · Title ID **PPSA99765** · **by YAHBALLON**.

The application integrates the *native* PS5 Flycast platform port from
[PSFlycast](https://github.com/Press5elect/PSFlycast), pinned at
`1cc6b2978d891f93cd2e36123ba4fd14118bd444`. The custom shelf uses
its working RADV/Vulkan game output, SH-4/AICA emulator, DualSense mapping,
VMU and GD-ROM media handling. This UI recreates the **visual style** of the
[PS5GBAX](https://github.com/Yahballon/PS5GBAX) / PS5SX2 3D cover-flow shelf;
it is not a byte-for-byte copy of the GBAX Vulkan renderer. The GBA core is
not used.

## Build

This app lives independently at `homebrew/ps5-dreamcastx/`. Its workflow is
`../../.github/workflows/build-ps5-dreamcastx.yml` and runs after app-specific
changes on `main`, for pull requests, or on manual dispatch. On Ubuntu 24.04
it installs build prerequisites, fetches pinned Flycast source, adapts the
frontend and title metadata, bootstraps PS5_Vulkan/RADV and SDK dependencies,
then compiles and verifies the real signed PS5 application.

On a successful **main push**, the workflow uploads `PPSA99765.zip`
to the independent tag `ps5-dreamcastx-v0.1.0`. PR builds create only
test artifacts. Releases are never created after a failing native build.
No ROM, BIOS or firmware is supplied. A source-only ZIP is **not** installable.

Run locally from the app directory (requires clang 18, CMake/Ninja, PS5 stack):

```bash
python3 scripts/prepare.py
bash scripts/bootstrap-ps5.sh
bash scripts/build.sh
```

Build output: `dist/PPSA99765.zip`. A heavy RADV dependency build may exceed
GitHub-hosted time/resources; inspect Actions logs if it does. Compilation is
not a substitute for testing games on a real PS5.

## Install

Extract the produced ZIP into `/data/homebrew/PPSA99765/` and launch through
a compatible PS5 homebrew loader. Put your **own legally obtained** Dreamcast
backups in the app's `games/` directory and, where needed, your own BIOS in
`bios/`. The native Flycast port also supports configured USB/SMB sources.

### DualSense

| Control | Action |
| --- | --- |
| D-pad / left stick | Navigate covers |
| Cross | Start selected game |
| Up / Down | Change shelf |
| Triangle | Game details |
| Square | Rescan library |
| L1 / R1 | Change source |
| Options | Settings |
| Circle | Back / exit dialog |

In-game menu, saves, disc switching and controller mapping follow PSFlycast.
Refer to its [manual](https://github.com/Press5elect/PSFlycast/blob/main/shell/ps5/MANUAL.md).

## Testing and limitations

The repo ships source transformation checks and a host C++ compile test.
Actual native compilation and physical-console testing must be tracked
separately. The native shell remains the upstream PSFlycast 1.1.0 implementation
with only app branding, shelf view, paths, icon and safe update-feed filtering
changed. Initial integration is **experimental**.

## Credits and licensing

Flycast and PSFlycast are GPL-2.0-or-later and retain their original notices;
original PS5GBAX/frontend reference is GPL-3.0-or-later. The new shelf overlay
is GPL-3.0-or-later; resulting combined distributions must respect all original
copyright and license notices. No commercial game assets or proprietary BIOS
files are included.
