# Audiobook Reader v0.2.2

## Fixed
- Extract embedded JPEG/PNG album artwork from DRM-free M4B (iTunes / MP4 cover streams) on a native background worker. Upload to OpenGL only on the UI thread.
- Show real artwork on the aluminum bookshelf and Now Playing screen, with proportional center-crop instead of title-only placeholder.
- Persist artwork under `/data/AudioBookReader/Covers/`; USB artwork is available after first playback/caching of the M4B file, including future launches.
- Improve local M4B discovery: scan nested subfolders under `/data/AudioBookReader/Books/` (up to six levels) and also accept loose .m4b files directly in `/data/AudioBookReader/` without scanning Cache or Covers.
- Add `SCAN_LOCAL_DETAIL` log entries with directory access errors, file counters, ignored tiny files, symlinks, and a sample filename.

## Install and diagnostics
Extract `PPSA99784.zip` to `/data/homebrew/`, overwriting the old install folder. Keep `/data/AudioBookReader` to preserve listening progress, USB cache and favorites. USB files belong in `/mnt/usb0/ABooks/` with a compatible running elfldr at TCP 9021. Local M4B files belong in `/data/AudioBookReader/Books/`. Press OPTIONS to rescan. If local books still do not appear, check `/data/AudioBookReader/audiobook-reader.log` for `SCAN_LOCAL_DETAIL`.

## Known limitations
Album artwork must be embedded in a supported JPEG/PNG/GIF attached picture stream. USB covers are cached after the audio file has first been copied/opened, not downloaded separately during the initial catalog scan. Console hardware playback still requires user verification.
