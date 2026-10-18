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
- Web settings remain accessible; QR generator no longer compiled.
- Embedded app version and package are generated from app.json.
