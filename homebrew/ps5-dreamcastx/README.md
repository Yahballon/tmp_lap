# PS5DreamcastX

**Dreamcast emulation on PS5 with a PS5GBAX-inspired animated cover-flow shelf.**

Version **0.1.3** · Title ID **PPSA99765** · **by YAHBALLON**.

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
to the independent tag `ps5-dreamcastx-v0.1.3`. PR builds create only
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

## App installation vs persistent data

The testbuild ZIP stays named `PPSA99765.zip` (title ID in PS5 metadata).
It contains one install directory, `PS5DreamcastX/`:

```text
/data/homebrew/PS5DreamcastX/eboot.bin       # PS5 executable
/data/homebrew/PS5DreamcastX/sce_sys/        # PS5 metadata/icon
```

**All emulator data is separate**, under `/data/PS5DreamcastX/`:

```text
/data/PS5DreamcastX/games/                  # internal games (CHD, GDI, CDI, CUE)
/data/PS5DreamcastX/bios/                   # optional BIOS
/data/PS5DreamcastX/covers/                 # downloaded and custom cover art
/data/PS5DreamcastX/data/                   # saves, VMUs, library/history
/data/PS5DreamcastX/data/savestates/        # save states
/data/PS5DreamcastX/vmu/                    # VMU export/import
/data/PS5DreamcastX/cheats/
/data/PS5DreamcastX/logs/
/data/PS5DreamcastX/frontend.cfg            # UI/USB settings
/data/PS5DreamcastX/emu.cfg                 # emulator settings
/data/PS5DreamcastX/network.cfg             # SMB library
```

The directories are created automatically at startup when filesystem
elevation succeeds. PS5DreamcastX tries `elfldr` at port **9021** before
selecting the data location, even when USB browsing is switched off.
If external /data cannot be opened, the app continues using its mounted
app directory as a fallback; check the boot log for `storage: app=... data=...`.
The empty-library screen shows the actual path used by the process.

**Migration from v0.1.1:** Before running this new build, back up the
existing `games/`, `bios/`, `covers/`, `data/` (saves),
`frontend.cfg`, `emu.cfg` and `network.cfg` from the old app
directory `/data/homebrew/PS5DreamcastX/` and copy/move them under
`/data/PS5DreamcastX/`. The app does **not** delete or automatically
move them. Leave `eboot.bin`, `sce_sys/`, `sce_module/` and
`BUILD.txt` under the installation directory.

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

### DVD-style cases (v0.1.3)

The game shelf now uses bounded flat DVD-style cases with narrow spines instead of perspective quads/floor shadows. Cover images preserve their original aspect ratio without cropping (square Dreamcast artwork is centered with a matching case matte). Existing USB and /data save paths are unchanged.
