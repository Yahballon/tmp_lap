# Open Surge PS5 · engine port

**Open Surge / Surge the Rabbit on native PS5 · by YAHBALLON · 0.1.0 (engineering bootstrap)**

Based on the complete [Open Surge Engine](https://github.com/alemart/opensurge), **not a rewritten or simulated platformer**. Upstream uses the [SurgeScript](https://github.com/alemart/surgescript) virtual machine, Allegro 5 graphics, input and audio, plus PhysicsFS asset mounting.

> **Current status — NOT PLAYABLE ON PS5.** No native PS5 `eboot.bin`, install ZIP, release, game UI screenshot or AI-generated PS5 icon is claimed. This project currently pins the upstream game engine, tests its Linux build, and adds an isolated PS5 entrypoint. The complete PS5 Allegro/SDL runtime and packaging must still be ported and tested.

## Full-port goal

Preserve the original gameplay, levels, sprites, sound effects, music, title screen, SurgeScript game logic, settings, level editor/MOD support where console-friendly, and save data. A simple launcher, static UI or graphics test is **not** an acceptable final replacement.

## Contents

| Path | Purpose |
| --- | --- |
| `upstream.lock.json` | Immutable upstream revision mapping |
| `scripts/fetch-upstream.sh` | Reproducible source checkout with SHA checks |
| `scripts/build-host.sh` | Build the original complete game engine on Linux |
| `ps5/ps5_launcher.h` | PS5 native launch arguments and persistent data path |
| `scripts/patch-ps5.py` | Carefully integrate PS5 entrypoint without deleting engine gameplay |
| `scripts/build-ps5.sh` | Fail-closed cross-build entrypoint requiring real PS5 Allegro/SurgeScript/PhysicsFS ports |
| `PORTING.md` | Technical blockers, bridge path and console acceptance tests |

## Host validation (Linux)

Install prerequisites, for example on Ubuntu 24.04:

```sh
sudo apt-get update
sudo apt-get install -y cmake ninja-build git gcc g++ \
    liballegro5-dev libphysfs-dev libgl1-mesa-dev \
    libfreetype-dev libpng-dev libjpeg-dev libvorbis-dev \
    libogg-dev libopus-dev
python3 -m unittest discover -s tests -p 'test_*.py' -v
bash scripts/build-host.sh
```

The host build compiles the **original, unmodified** Open Surge engine and its pinned SurgeScript VM. It does **not** establish PS5 functionality.

The native cross-build entrypoint `scripts/build-ps5.sh` requires a verified PS5 toolchain and library prefix. It will fail rather than manufacture a non-working `PPSA99358.zip`.

## Planned PS5 paths

- App: `/data/homebrew/PPSA99358/`
- Game assets: `/data/homebrew/PPSA99358/game/`
- Persistent data: `/data/OpenSurge/` (must be verified on hardware)
- Planned identity: `PPSA99358`; proposed only, **not** Sony-assigned or globally reserved

DualSense target mapping: D-pad/left stick move, Cross jump/confirm, Circle back, Options pause. Not tested or integrated into a PS5 build yet.

## Release / compatibility

There is **no playable PS5 package**. Firmware 5.10 support has **not been tested**. This repository intentionally does not publish a fake test build. See [PORTING.md](PORTING.md) for the remaining native work.

## Credits / license

Open Surge © Alexandre Martins and contributors — GPLv3; SurgeScript by Alexandre Martins and contributors; Allegro and PhysicsFS by their respective authors. Source-port integration © 2026 YAHBALLON — GPL-3.0-or-later. Respect all transitive dependencies and artwork license files when packaging a playable build.
