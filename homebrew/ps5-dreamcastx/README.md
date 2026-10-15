# PS5DreamcastX

**Dreamcast emulation on PS5 with a PS5GBAX-inspired animated cover-flow shelf.**

Version **0.1.1** · Title ID **PPSA99765** · **by YAHBALLON**.

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
to the independent tag `ps5-dreamcastx-v0.1.1`. PR builds create only
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

## Install & game locations

The ZIP **remains named `PPSA99765.zip`** for the TESTBUILDS catalog.
The **folder inside** it is now `PS5DreamcastX/`, rather than `PPSA99765/`.
The PS5 Title ID **remains `PPSA99765`** in `sce_sys/param.json` (required by PS5 loaders).

Use a compatible ShadowMount loader to install and launch:

```text
/data/homebrew/PS5DreamcastX/eboot.bin
/data/homebrew/PS5DreamcastX/games/<your-legally-obtained-backup>.chd
/data/homebrew/PS5DreamcastX/bios/
/data/homebrew/PS5DreamcastX/data/       # Saves and VMUs
```

### USB games

Plug a USB drive into the PS5. The application discovers
`PS5DreamcastX` case-insensitively on any of `/mnt/usb0/` through
`/mnt/usb7/` and scans the **games/** subfolder if present; otherwise
it scans the parent `PS5DreamcastX/` directory. Existing
`flycast/`, `dreamcast/`, `dc/`, `naomi/`, `atomiswave/`, and
`arcade/` USB folders remain supported.

```text
/mnt/usb0/PS5DreamcastX/games/Game.chd
/mnt/usb0/PS5DreamcastX/games/Game.gdi
/mnt/usb1/PS5DreamcastX/AnotherGame.cdi  # if games/ absent
```

USB access **requires filesystem elevation** (a jailbreak with
`elfldr` listening on port **9021**) and the app's
**Settings > Library > USB drives** enabled, followed by an app restart.
New installations default to enabling USB; an existing `frontend.cfg`
with `usb = 0` retains that preference until manually enabled.
In the library use **L1/R1** to select the USB tab and **Square** to rescan.

### Upgrading from v0.1.0

The old app installed at `/data/homebrew/PPSA99765/`. Stop the app,
then **move/rename that existing directory to**
`/data/homebrew/PS5DreamcastX/` over FTP/file manager. This preserves
`data/` saves, `games/`, `bios/`, and `covers/`. Extract the
v0.1.1 `PS5DreamcastX/` folder into that new location (overwrite
app binaries/assets, **not** your games and saves). Do not leave two
active copies with the same Title ID. The old v0.1.0 self-updater
expects a `PPSA99765/` ZIP and **cannot perform this folder migration**:
install v0.1.1 manually or through a TESTBUILDS installer that honors the
folder name inside the ZIP.

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
