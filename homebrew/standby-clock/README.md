<p align="center"><img src="assets/icon.png" alt="Standby Clock icon" width="150"></p>

# STANDBY CLOCK

**Native PS5 Homebrew by YAHBALLON · v0.1.0**

A minimal, television-friendly always-on clock based on a black and red reference design. The clock is **actually drawn every frame** by the native PS5 UI renderer—not a static picture. The display includes a huge digital clock, analog sweep-second hand, date, calendar week, alarm tile and Homebrew status. The application does not control actual PS5 power or suspend functions.

**Status:** Source implementation prepared for CI. PS5 hardware capture and console test pending.

## Install / TESTBUILD

Title ID: `PPSA99876` · release tag (when published): `standby-clock-v0.1.0`.

After a successful main-branch GitHub Actions build, the verified release asset `PPSA99876.zip` is discovered by the existing `tools/update-version.py` catalog generator and can be loaded in the TESTBUILD app. Do not manually edit `version.json`.

## Controller

| Control | Clock screen | Settings |
| --- | --- | --- |
| X | Open settings / dismiss alarm | Change option |
| Square | Open alarm settings | — |
| Triangle | Cycle color theme | — |
| L1 / R1 | Brightness down/up | — |
| Options | Settings | Save and return |
| D-pad / Left stick | — | Navigate or change values |
| Circle | Exit (or snooze alarm five minutes) | Save and return |

## Settings & storage

The app reads the device clock using `time()` and localtime, with optional fixed UTC-hour offsets; correct time depends on the PS5's clock/timezone. Persistent preferences are written to `/data/StandbyClock/settings.ini`; diagnostic log: `/data/StandbyClock/clock.log`. OLED pixel shifting moves content slightly every minute. Brightness is an in-app dim overlay—not a system display setting.

## Build

Workflow: [build-standby-clock.yml](../../.github/workflows/build-standby-clock.yml). Uses the PS5-DevBridge simple-calc native project and Yahballon ps5-homebrew-ui renderer with pinned OpenGL SDK fork. It assembles `PPSA99876/eboot.bin`, `sce_sys/icon0.png`, metadata, assets and modules. GitHub Actions runs a C++20 unit test and validates the installable ZIP. The launcher icon is generated with Pillow from [generate_icon.py](assets/generate_icon.py).

## Limitations

The alarm only triggers while this app is open; no system-wide wake notifications, background operation, automatic standby activation, or alarm audio. The clock is rendered using the framework's bundled Inter font, whose PS5 rendering size must be validated on hardware. This repository includes no Sony proprietary assets.

Development handoff: [MILESTONES.md](MILESTONES.md). MIT app code, subject to separate upstream renderer/SDK licenses.
