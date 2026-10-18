# PS5NESX TESTBUILDS port
Source snapshot from https://github.com/Yahballon/PS5NESX.
The native app is in `nes/` and uses `ps5/` and Vulkan headers in `3rdparty/`.
The unrelated PCSX2 desktop scaffold is excluded deliberately.
Edit this folder in tmp_lap, update `app.json` to the next SemVer
(e.g. 1.0.10), and run **Build PS5NESX Native**.
The resulting GitHub Release is named `ps5nesx-v<version>`
with the installable `PPSA99204.zip` and is discovered by `version.json`.
`1.0.9` is an unchanged rehost of the upstream `v1.0.0.9` binary.
Never bundle commercial NES ROMs.

## Classic 3D CoverFlow (v1.0.10)

Native, Wii-era-inspired perspective placement with a 128%-scale selected
cover, inward-rotated and receding neighbours, reflective floor, and the
selection title at the top. Geometry is in
`ps5/frontend/fe_coverflow_layout.h` (shared/reusable, no Wii binaries or
assets). Animations, NES input mappings, cover caching and settings remain
on the existing PS5 renderer.

Run the dependency-free host layout test:

```sh
c++ -std=c++20 -O2 -Wall -Wextra -Werror \
  -Ips5/frontend tests/coverflow_layout_test.cpp -o /tmp/ps5nesx-layout-test
/tmp/ps5nesx-layout-test
```

**Catalog limitation:** The installed TESTBUILDS PS5 parser rejected the
16-app manifest when PS5NESX was added. While that issue remains open, new
PS5NESX releases are directly installable from tmp_lap GitHub Releases
but are intentionally omitted from version.json.

## Embedded version fix (v1.0.11)
The upstream `nes/src/main_shelf.cpp` had four hardcoded `v1.0.0.9`
strings for the on-screen build tag, log header and web server.
`nes/tools/build_shelf.py` now automatically regenerates
`nes/src/ps5nes_build_version.h` from `app.json` before compilation
and synchronizes `sce_sys/param.json`, the package filename, and
`build-manifest.json`. GitHub Actions also verifies the packaged
manifest and title version before publishing.

## v1.0.12 - QR-free shelf and title placement
- Move selected game title below the central 3D cover, above ROM metadata.
- Remove corner QR tile, large Triangle QR overlay and QR controller hint.
- Web settings remain accessible; the QR encoder stays linked internally for compatibility.
- Embedded app version and package are generated from app.json.

## Deterministic Lapy packaging
The original pinned Lapy upstream is currently unavailable. The build uses only
`PPSA99204/lapy.elf` and its manifest from this repository's already signed
`ps5nesx-v1.0.11` ZIP, with an exact SHA-256 archive pin, and verifies the
helper's ELF signature, title ID and per-file hash. This does not overwrite
the newly compiled and signed `eboot.bin` for v1.0.12.

## v1.0.13 — Experimental Mesen-format HD texture packs

The vendored FCEUmm includes a Mesen `hires.txt` HD pack engine (formats
through v109). PS5NESX previously disabled it with `HAVE_HDPACK := 0`.
This opt-in test build enables HD-pack rendering, graphics/audio decoding
and the wider HD-frame path. HD pack execution has **not yet** been tested
on physical PS5 hardware.

Install an extracted pack with a matching ROM file (never bundle commercial
ROMs or unlicensed artwork):

```text
/data/PS5NES/games/Zelda.nes
/data/PS5NES/HdPacks/Zelda/hires.txt
/data/PS5NES/HdPacks/Zelda/<referenced assets>
```

In the game Options (Square), Emulation > HD texture packs > On, then
start the ROM. The setting defaults Off to preserve vanilla gameplay.
Individual-game or global preferences are supported. `[HDPACK]` in
`/data/PS5NES/logs/boot.log` records enable state, expected path,
and whether the manifest exists. Automatic fallback uses normal NES
emulation without a matching pack. Maximum video callback 2560x2400;
Integer mode downscales a larger image to fit the screen.

Test on PS5 with a legally sourced Mesen-format HD pack. Newer pack
formats, performance and audio compatibility are unverified.

## v1.0.14 — HD edge and HUD flicker regression build

The SMB1 4x HD pack showed glitches at the left border and below the in-game HUD.
Suspected causes are fixed by guarded tile caching, a proper left-edge mask,
and complete HD-frame clearing. These are code-level changes, not a visual
guarantee without testing on physical PS5 hardware.

- Scanline tile cache now validates the entire tile key (CHR index/data
  and palette) before reusing a match. HUD splits can switch tiles within
  the previous 8-pixel fast-cache window.
- The first 8 pixels respect the NES background-mask palette when that
  mask is enabled; no old XBuf colours are allowed into that strip.
- Each HD frame buffer is fully cleared before composition; source frames
  are converted across every pixel and the frame geometry and pitch logged.
- View /data/PS5NES/logs/boot.log for [HDPACK] HD compose: and
  [HDPACK] video_frame= messages.

Normal NES emulation, HD pack directories, saves and the coverflow stay intact.
Compare the same World 1-1 HUD/left-edge scene on v1.0.13 versus v1.0.14.
