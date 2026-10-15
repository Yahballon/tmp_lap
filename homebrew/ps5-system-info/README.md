# PS5 System Info

**PS5 SYSTEM INFO by YAHBALLON** - Native system dashboard v0.1.0.

- Title ID: `PPSA99786`
- Content ID: `UP9000-PPSA99786_00-PS5SYSTEMINFO001`
- Release tag: `ps5-system-info-v0.1.0`
- Source: `homebrew/ps5-system-info/`
- Build: `.github/workflows/build-ps5-system-info.yml`

## Features

A 1920x1080 dark-blue native OpenGL dashboard inspired by the supplied reference.
Sections: Overview, System, Hardware, Storage, Network, Display / Video, Audio,
Power & Thermals, Peripherals, Installed Apps, Processes, Kernel / Debug, Logs, Settings.

Uses real read-only readings where a supported API exists: application/system clock,
monotonic uptime, current process ID and CPU time, controller connectivity,
render framebuffer size, local route IPv4, accessible paths, installed *homebrew*
folders and their executable sizes. Full homebrew list has scrolling.
Native per-app logs, versioned persistent preferences and atomic text-report export.

**Important limitations:** Most kernel-level and privileged Sony console APIs
are not available or documented through the current portable runtime.
Firmware revision, model number, global CPU/GPU use, RAM/VRAM pressure,
thermals, fan RPM, SSD capacity, EDID, HDR/VRR state, retail games,
NAT type, peripheral battery and other system processes show **Unavailable**
rather than a fabricated reading. Hardware architecture descriptions are
static nominal specifications, not live hardware measurements.
Network IPv4 is the locally chosen outgoing interface; it is neither
a public IP lookup nor proof of Internet connectivity.
Monotonic uptime may not equal time since console power-on.

## Controller

| Button | Action |
| --- | --- |
| D-pad Left/Right | Change section |
| D-pad Up/Down | Scroll current detail, installed apps or settings; move sections on Overview |
| Left Stick | Direction navigation with repeat |
| L1 / R1 | Previous / next section |
| Cross | Refresh or change setting |
| Circle | Return to Overview / quit |
| Triangle | Refresh snapshot |
| Square | Export text report |
| Options | Settings |

## Settings and logging

Preferences: `/data/ps5-system-info/settings.ini`
Diagnostics: `/data/ps5-system-info/system-info.log`
Report: `/data/ps5-system-info/report.txt`
Atomic .tmp replacement for settings and exports. No privileged offsets,
external network API requests or console-changing functions.

## Installation

Download `PPSA99786.zip` from [Releases](https://github.com/Yahballon/tmp_lap/releases/tag/ps5-system-info-v0.1.0).
Extract `PPSA99786/` into `/data/homebrew/` on your PS5, then register
the title with a compatible homebrew loader, such as ShadowMountPlus.

## Building

GitHub Actions `Build PS5 System Info` uses the
[PS5-DevBridge](https://github.com/Yahballon/PS5-DevBridge)
simple-calc application scaffold and its native PS5 OpenGL toolchain.
Workflow validates host-side diagnostics and archive content.

## Licensing

GPL-3.0-or-later, consistent with the UI and native scaffold.
Generated artwork is original. No Sony or PlayStation logos included.
