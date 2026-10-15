# Audiobook Reader for PS5

A native, controller-first M4B audiobook player by YAHBALLON, inspired by the bookshelf experience of the [PS5 Ebook Reader](https://github.com/Yahballon/PS5_Ebook_Reader). This application is separate from the Ebook Reader.

## Features

- USB ABooks folder scanned via a dedicated read-only `abookscan.elf` helper launched through active `elfldr` on `127.0.0.1:9021` (USB helper port `9177`).
- Async transfer to `/data/AudioBookReader/Cache/` with progress percentage, cancellation, interrupted-transfer retry, and safe file-size validation.
- Application diagnostics at `/data/AudioBookReader/audiobook-reader.log`; persistent listening state at `/data/AudioBookReader/progress.tsv`.

- Brushed aluminum bookshelf, stainless steel rails and cyan focus highlights.
- Scans M4B audiobooks recursively (up to three levels deep) from the two dedicated sources: local `Books` and USB `ABooks`.
- Native FFmpeg AAC decoder with SDL2 audio output.
- Play/pause, skip ±15/30 seconds, seek ±60 seconds, chapter navigation (embedded MP4 chapters).
- Favorites, All and In Progress views, grid/list view and resume playback.
- Saves playback positions and favorite flags in `/data/AudioBookReader/progress.tsv` with atomic rename.
- Clear exit confirmation; saves when returning to library and periodically while playing.

## Controller

| Button | Library | Playback |
| --- | --- | --- |
| D-Pad | Navigate | Left −15s / Right +30s |
| Cross | Play selected book | Play / Pause |
| Circle | Exit prompt | Back to library |
| Triangle | — | Toggle chapters |
| Square | Favorite | Jump to highlighted chapter |
| L1 / R1 | Filters | Previous / Next chapter |
| L2 / R2 | — / Grid-list switch | −60s / +60s |
| Options | Rescan library | — |

## Install

Download `PPSA99784.zip` from [Releases](https://github.com/Yahballon/tmp_lap/releases/tag/audiobook-reader-v0.2.0) after a successful CI release. Extract the `PPSA99784` folder to `/data/homebrew/`, register it with your supported loader, and copy DRM-free `.m4b` files into `/mnt/usb0/ABooks/` on USB or `/data/AudioBookReader/Books/` locally (optional: `/download0/ABooks/`).

USB books are enumerated by the included `abookscan.elf` helper and need an already-running elfldr on TCP port 9021. The app transfers selected USB M4B files to `/data/AudioBookReader/Cache` before decoding; originals on USB remain untouched. If the helper cannot start, local books remain usable and the app shows its USB status. Encrypted FairPlay M4B audiobooks cannot be decoded. Embedded chapters require compatible MP4 chapter metadata. Audio output requires the FFmpeg/SDL2 libraries available in the PS5-DevBridge toolchain.

## Build

Run `.github/workflows/build-audiobook-reader.yml` via GitHub Actions; uses `PS5-DevBridge/templates/simple-calc`, native `ps5-homebrew-ui` OpenGL renderer, FFmpeg and SDL2 PacBrew dependencies. The workflow packages and validates executable, assets, metadata and release archive. Host smoke tests cover library scanning and progress persistence.

**Version:** v0.2.0  
**Title ID:** PPSA99784  
**Content ID:** UP9000-PPSA99784_00-AUDIOBOOKREADER1

License: GPL-3.0-or-later. Code derived from the GPL-compatible PS5 homebrew UI and development boilerplate.
