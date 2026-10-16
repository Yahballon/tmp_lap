# PS5MesenX — MesenCE PS5 port workspace

**Status:** Native PS5 TESTBUILD v0.1.2. Compiles and packages successfully; ROM playback and HD packs still need on-console testing.

This independent workspace preserves the existing PS5SNESX application and imports
the PS5 frontend and Mesen Community Edition sources at pinned revisions.
The native PS5 core and frontend are compiled into a signed SELF, but actual PS5 compatibility requires testing.

## Upstreams

- PS5SNESX frontend: https://github.com/Yahballon/PS5SNESX
- Mesen Community Edition: https://github.com/nesdev-org/MesenCE
- Both GPL-3.0 projects; retain all original copyright, license and notices.

Run the **Sync PS5MesenX upstream sources** GitHub Actions workflow to vendor
the pinned sources into `homebrew/ps5-mesenx/upstream/`. The workflow does not
overwrite the existing PS5SNESX repository or other homebrew projects.

## Goal: NES HD Packs

Zelda: Remastered is an **NES** HD pack, not an SNES ROM. MesenCE already
has `Core/NES/HdPacks/HdPackLoader.cpp`, `HdNesPpu`, high-resolution video
and HD audio facilities, but these are **not yet wired to the PS5 frontend**.

PS5 paths (v0.1.2):

```text
/data/PS5NES/games/The Legend of Zelda.nes
/data/PS5MesenX/HdPacks/The Legend of Zelda/hires.txt
/data/PS5MesenX/HdPacks/The Legend of Zelda/<graphics-and-audio-assets>
/mnt/usb0/PS5MesenX/games/nes/
/mnt/usb0/PS5MesenX/HdPacks/
```

The pack folder must match the ROM filename without its extension; follow
the pack author's ROM revision/hash requirements. Zelda: Remastered is
designed for a specific US NES revision, not arbitrary Zelda ROMs.

To prepare an HD pack on a PC:

```bash
python3 homebrew/ps5-mesenx/scripts/hdpack.py install \
  ./downloaded-hd-pack.zip --rom "The Legend of Zelda.nes" \
  --output ./PS5MesenX/HdPacks
```

The tool validates the archive structure, rejects traversal/symlinks and
extracts into the matching game directory. This is a *staging tool*, not
proof of PS5 playback support. No commercial ROMs, copyright-protected
game art or third-party HD packs are bundled.

## Current milestones

1. [x] Define isolated project and pinned upstream source-import workflow.
2. [x] Provide safe HD pack staging and automated tests.
3. [x] Compile the MesenCE native C++ core for PS5 using the SDK.
4. [x] Implement the MesenCE-to-PS5 session adapter, callbacks and storage.
5. [ ] Wire video frames above 512x576, OGG audio, DualSense and SRAM/states.
6. [ ] Load verified NES HD packs on an actual PS5.
7. [x] Produce signed app ZIP, release and TESTBUILDS version.json entry.

Read [PORTING.md](PORTING.md) before changing the emulation backend.
\n## Native adapter development (October 2026)\n\n`src/core_session.{h,cpp}` implements an **experimental** native C++ MesenCE\nbridge with NES ROM staging, HD pack lookup, HD-safe video frame buffering,\n48 kHz stereo capture, controller input and Mesen-native save states. It is\nnot yet included in a PS5 `eboot.bin`. The `Verify PS5MesenX native bridge`\nworkflow compiles its interface on a Linux host and checks upstream MesenCE\nbuildability; passing Linux tests does not verify PS5 compilation or playback.\n\nA separate native PS5 packaging workflow and hardware verification are the\nnext gates before any app should appear in `version.json`.\n
## Shared ROM library (v0.1.2)

PS5MesenX scans `/data/PS5NES/games` as its **primary NES ROM directory** and creates it at startup if missing. Game files must be `.nes`. Existing saves, settings, covers, logs, ROM cache and HD packs remain isolated in `/data/PS5MesenX`, and the original PS5SNESX repository is not modified.

## ROM detection and troubleshooting (v0.1.2)

Both `/data/PS5NES/games` and `/data/homebrew/PPSA99896/roms` are scanned, along with legacy game folders and USB locations. Supported ROMs are plain `.nes` files or ZIP archives containing exactly one `.nes` file with a valid iNES/NES2.0 header. Subfolders are scanned to depth 5.

If no games show up, check `/data/PS5MesenX/logs/boot.log` for `[romscan] entry`, `[library] scan source`, `[library] ROM FOUND`, or `cannot open` lines, and confirm filesystem elevation succeeded. If root storage is unavailable the fallback log may be under `/download0/PS5MesenX/logs/boot.log`. Existing ROMs and saves are never moved or deleted.
