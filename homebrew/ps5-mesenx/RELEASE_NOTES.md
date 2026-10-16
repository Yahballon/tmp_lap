# PS5MesenX v0.1.8 — Experimental PS5 NES HD Emulator

First **native PS5 port experiment** using MesenCE Libretro (based on Mesen
Community Edition) and the PS5SNESX-derived Vulkan game shelf. This version
is intended for testing on jailbroken PS5 consoles, not production use.
Untested on real PS5 hardware.

**Expected capabilities:** NES ROM scanning/loading, DualSense controls,
local game saves, stereo audio and Mesen-compatible high-resolution NES HD
packs. These are intended behaviors, not guarantees of console compatibility.

**Install folder:** `/data/homebrew/PS5MesenX/` (from `PPSA99896.zip`).
**Title ID:** `PPSA99896`. Copy the full directory and keep its `sce_module`.
**ROMs:** `/data/PS5NES/games/*.nes` (shared NES library) or
`/data/homebrew/PPSA99896/roms/*.nes` or the matching *.zip files. USB NES ROMs remain supported at `/mnt/usb0/PS5MesenX/games/`. ZIPs must contain exactly one NES ROM (NFS disabled).
**Saves, covers, settings, logs and HD packs stay separate under** `/data/PS5MesenX/`.

**HD packs:** `/data/PS5MesenX/HdPacks/<ROM filename without .nes>/hires.txt`.
HD pack assets must meet the pack author's ROM hash/revision requirements.

**ROM scanner diagnostics:** Look for `[romscan]`, `[library] scan source`, and `[library] ROM FOUND` lines in

**What to report:** boot output in `/data/PS5MesenX/logs/boot.log`,
startup success, NES video/audio, buttons, and HD pack loading.
There are no ROMs, firmware or proprietary HD packs in the archive.

**Safety gates:** A release is permitted only if the genuine MesenCE core
cross-compiles for x86_64-sie-ps5, links with the PS5 Vulkan frontend,
the PS5 native signer emits `eboot.bin`, the SELF magic is verified,
and `PPSA99896.zip` passes integrity and content checks.

Credits: MesenCE/MesenCE-Libretro, PS5SNESX, original PS5SX2 frontend,
public PS5 Payload SDK and Vulkan contributors. GPL-3.0 sources and notices.

### v0.1.6 fixes

- Scans `/data/PS5NES/games` and `/data/homebrew/PPSA99896/roms`.
- Accepts `.nes` and `.zip` files containing exactly one valid NES ROM.
- Uses a NES-only library cache rather than the old SNES extension validator.
- Writes directory visibility, scan sources, ROM counts and permission errors to boot.log.

### v0.1.6 scanner fix

Fixed PS5 filesystem metadata compatibility: try stat and fstatat, then dirent d_type fallback for NES games visible to PS5NES. Log every discovered NES file. Saves and ROMs are untouched.

### v0.1.6: PS5NESX proven scanner integration
- Reused PS5NESX filesystem ROM scanning with DT_REG/DT_UNKNOWN and descriptor metadata fallbacks
- Direct ROM header checks when path metadata is denied
- Bounded stream reads without requiring fstat to work
- Retains shared /data/PS5NES/games and /data/homebrew/PPSA99896/roms

### v0.1.6: Never silently use an empty cache
- Force a fresh PS5NESX ROM directory scan every app startup, preserving its live progress.
- Change hardcoded loading header to PS5MesenX and v0.1.6.
- Low-level scan diagnostics append to /data/PS5MesenX/logs/boot.log, with fallback at /download0/PS5MesenX/logs/boot.log and /data/homebrew/PPSA99896/mesenx-debug.log.

### v0.1.6 diagnostics
- Preserve early BOOT and ELEVATION lines by not truncating boot.log during startup
- Log DIR_OPEN, DIR_FAIL (errno), DIR_ENTRY and DIR_DONE per directory
- Distinguish unavailable USB folders from the primary shared NES ROM path

### v0.1.8: Real filesystem permission fix
- Console reported ELEVATION filesystem=5 (unavailable), with EPERM opening NES ROM directories
- Restored the original PS5NESX title-bound one-request Lapy helper using the pinned upstream builder and SDK
- Includes verified lapy.elf, lapy-manifest.json and Lapy-MIT.txt license in installation ZIP
- Displays filesystem permission errors instead of misleading 0-game success if elevation remains unavailable
- Logs elevation status plus selected resident/helper path; no ROMs are modified

### v0.1.8 Video output compatibility
- MesenCE decoded-frame fallback if the headless renderer fails to publish the video frame.
- Disable video frame duplication to ensure real frames arrive at the PS5 host.
- Enable MesenCE FrameProbe and log VIDEO_CALLBACK, VIDEO_NULL_FRAME, and VIDEO_STATUS to boot.log.
- Audio and ROM scanner remain unchanged.
