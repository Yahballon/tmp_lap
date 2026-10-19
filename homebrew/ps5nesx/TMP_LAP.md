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

## v1.0.15 — Porpoise-inspired CoverFlow and authentic menu sounds

The 3D shelf now follows the shallow-yaw, wide-spacing, shrinking outer case
geometry in elripalda/Porpoise-Dolphin-Emulator-for-PS5, src/ui_app_views.cpp,
(c) 2026 Ruben (@elripalda), GPL-3.0-or-later.
The existing PS5NESX renderer, floor reflections and cover handling are retained.

An unobtrusive blurred/dimmed selected-cover backdrop is sampled through
the existing Vulkan descriptor for that game. The fallback sampler is a
standalone white pixel (never the scene render target). The actual background
shader SPIR-V is rebuilt from shaders/bg.frag on every PS5 CI build.

Authentic Porpoise game-row.wav and menu-scroll.wav (copyright Ruben) are
downloaded from source commit 2ed1ab5a06995aa198f7bccd4da0f90c3fc29d39
and verified by their Git blob hashes in CI. These are embedded inside the
signed eboot.bin with .incbin; NO live runtime download or extra installed
assets are needed. The frontend mixer decodes 48 kHz 16-bit stereo RIFF WAVs
and uses the original synthesized menu sounds if audio decoding fails.

Credits/source: https://github.com/elripalda/Porpoise-Dolphin-Emulator-for-PS5
Licence: GPL-3.0-or-later. No original Dolphin or console brand graphics are
copied. This is a PS5NESX adaptation, not the entire Porpoise frontend.
Requires on-console verification of blurred background, cover transitions,
sound panning, memory use and game launch.

## v1.0.19 — Minimal header, working settings, auto NES covers and wrap

The original Porpoise library overlay hid the native PS5NESX settings sheet.
The frontend now suspends the full-screen Porpoise renderer whenever the sheet
is open or animating (Square), returning to the glass coverflow when closed.
No false/inert 'Settings' header tab is shown anymore. **Removed entirely:
Library / Save States / Settings top tabs and L1/R1 header icons.**
The PS5NES wordmark, clock, blue glass cases and floor remain.

Missing NES cover images download asynchronously using the game's title and
the two already supported public artwork sources. A common three-digit ROM
collection prefix (e.g. '022 Rockman 1') is stripped from lookup keys, while
the original ROM path/title is unchanged. Local/manual covers remain first
priority; successful network images are checked, cached atomically at
`/data/PS5NES/cache/<NES-ROM-CRC>.jpg` and loaded offline on next boot.
Despite the .jpg cache suffix, both JPEG and PNG bytes are accepted by the
existing stb_image decoder. Never fetch ROMs. Download availability depends
on user internet connection and the external artwork archives.

D-pad right on the last game wraps to game 1; D-pad left on game 1 wraps to
the last. The same applies to L1/R1 five-game jumps. At a wrap, the carousel
snaps to the new index so it doesn't animate through the whole catalogue.
Settings, saves, HD NES emulation and sound are preserved. Console testing
should verify the settings sheet, fresh cover network access and wrap.


## v1.0.20 — Porpoise Settings overlay + auto cover progress and full library cache

- The original Porpoise blue-glass menu now remains visible *behind* the
  Settings sheet. The old black NES 3D cases and legacy menu headers are
  removed from the native scene while Porpoise is active.
- Vulkan draw order is corrected: draw Porpoise first, then the native
  Settings/keyboard UI with explicit pipeline + vertex-buffer rebinding.
- Compact Porpoise-themed cover progress widget at the upper-right reports
  selected-game download in flight, completed requests, saved covers and
  failures. The moving blue glint means the worker is still transferring.
- The cover worker checks the complete ROM library and continues downloading
  off-screen missing covers to disk, prioritizing the currently visible ones.
  The GPU still only uploads nearby covers. Downloads remain in
  \`/data/PS5NES/cache/<ROM-CRC>.jpg\` and reuse the disk on next boot.
- A third, optional NES image source (ArcadeArtwork Piwigo album #547,
  https://www.arcadeartwork.org/index.php?/category/547) is queried once
  per worker session only if the earlier GitHub repositories have no matching
  cover. The site Piwigo API returns category image metadata and validated
  image URLs. No guessed URLs or HTML are ever persisted as artwork.
  This feature is best effort: access may be blocked by Cloudflare or the
  site owner, in which case existing providers keep working.
- Negative 404 markers are v9 and expire within 48 hours, allowing retries.
  Manual artwork always wins; no ROM downloading is involved.

Real PS5 testing is needed to confirm the overlay, progress UI and online
gallery access. Host integration tests validate parsing, safe URL hosts,
download-index caching, wrap navigation, and draw ordering.

## v1.0.21 — Official Libretro NES Boxarts source and transient progress

The user-selected [official NES thumbnails directory](https://thumbnails.libretro.com/Nintendo%20-%20Nintendo%20Entertainment%20System/Named_Boxarts/)
is now a first-class cover source. The worker downloads the archive's
**actual directory index once per session** (when any artwork is missing)
and matches game titles after removing numbered collection prefixes. It
prefers region-appropriate official filenames, then tries direct Libretro
CDN links, the existing artwork repository and Github mirror, followed
by ArcadeArtwork when needed. It never downloads ROMs.

The on-screen compact Porpoise cover progress HUD is only visible during
an active pending search/transfer. On completion or when network access
stops the session, it disappears automatically. R2 manual refresh starts
a fresh progress cycle.

All successful artwork downloads are decoded and written atomically to
`/data/PS5NES/cache/<NES-CRC>.jpg`; existing cached/manual covers still
load before network access. Previous v9 negative-hit markers no longer
block rechecking from the new source: v10 has independent 48-hour markers.
One index fetch per worker means the entire 352-game batch need not try
dozens of dead aliases before discovering the actual filename.

The exact yield depends on the names/regions in the user's game library.
A successful build does not guarantee all 352 games have published box
images or that Libretro CDN/TLS access succeeds on PS5; the download log
records each URL, HTTP status and cache writes.
