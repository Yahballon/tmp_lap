# PS5MesenX v0.1.1 — Experimental PS5 NES HD Emulator

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
`/mnt/usb0/PS5MesenX/games/*.nes`. Only uncompressed NES ROMs are enabled
in this first test (ZIP disabled; NFS disabled).
**Saves, covers, settings, logs and HD packs stay separate under** `/data/PS5MesenX/`.

**HD packs:** `/data/PS5MesenX/HdPacks/<ROM filename without .nes>/hires.txt`.
HD pack assets must meet the pack author's ROM hash/revision requirements.

**What to report:** boot output in `/data/PS5MesenX/logs/boot.log`,
startup success, NES video/audio, buttons, and HD pack loading.
There are no ROMs, firmware or proprietary HD packs in the archive.

**Safety gates:** A release is permitted only if the genuine MesenCE core
cross-compiles for x86_64-sie-ps5, links with the PS5 Vulkan frontend,
the PS5 native signer emits `eboot.bin`, the SELF magic is verified,
and `PPSA99896.zip` passes integrity and content checks.

Credits: MesenCE/MesenCE-Libretro, PS5SNESX, original PS5SX2 frontend,
public PS5 Payload SDK and Vulkan contributors. GPL-3.0 sources and notices.
