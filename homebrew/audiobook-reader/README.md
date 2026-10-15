# Audiobook Reader for PS5

A native, controller-first M4B audiobook player by YAHBALLON, inspired by the bookshelf experience of the [PS5 Ebook Reader](https://github.com/Yahballon/PS5_Ebook_Reader). This application is separate from the Ebook Reader.

## HD PS5 Icon

![Audiobook Reader HD neon icon](sce_sys/icon0.png)

The new 512x512 neon bookshelf/headphones artwork replaces the former upscaled 48px app icon. It is embedded in the PS5 home-screen icon and the packaged app asset.

## Features

- **Live chapter tracking:** The Now Playing chapter number and title follow the decoded playback position automatically, including natural chapter changes, forward/backward seeks, and resume. The chapter list marks the currently playing chapter without overriding manual selection while browsing.
- **Library selection fix:** Pressing Cross opens the currently highlighted audiobook, not the first title when the shelf contains multiple items.


- **Local Books permission fix:** If the PS5 app cannot enumerate `/data/AudioBookReader/Books` (errno 1 / EPERM), the helper scans the folder and transfers selected books to the app's playable cache; local originals remain unchanged.
- Local helper protocol uses `LISTLOCAL` and `FETCHLOCAL` on the dedicated port 9179 through the existing elfldr on port 9021.
- **Cover fallback chain:** FFmpeg M4B attached-picture stream, direct iTunes `covr` MP4 atom parsing, then adjacent `cover.jpg`, `folder.jpg`, `front.jpg`, `cover.png`, or `<audiobook-name>.jpg/.jpeg/.png`.
- Companion artwork works on USB `ABooks` and local `Books`, without transferring the entire M4B first.
- New diagnostics: `LOCAL_HELPER_BEGIN`, `LOCAL_HELPER_DONE`, `LOCAL_HELPER_ERROR`, `COVER_COMPANION` and `COVER_MISSING`.



- M4B embedded album art (iTunes/MP4 covr as JPEG or PNG) decoded with native FFmpeg + stb_image, shown on the aluminum bookshelf and player.
- Persistent artwork cache under `/data/AudioBookReader/Covers/`; USB covers appear after the book is first cached or opened, then persist across restarts.
- Extended local `.m4b` scanning (recursive up to six levels) of `/data/AudioBookReader/Books/` and loose files in `/data/AudioBookReader/` without scanning Cache/Covers.
- `SCAN_LOCAL_DETAIL` diagnostics in `audiobook-reader.log`: scanned directory, file count, accepted M4B count, skipped small files, sample entry and errors.


- USB ABooks folder scanned via a dedicated read-only `abookscan.elf` helper launched through active `elfldr` on `127.0.0.1:9021` (USB helper port `9179`).
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

Download `PPSA99784.zip` from [Releases](https://github.com/Yahballon/tmp_lap/releases/tag/audiobook-reader-v0.2.5) after a successful CI release. Extract the `PPSA99784` folder to `/data/homebrew/`, register it with your supported loader, and copy DRM-free `.m4b` files into `/mnt/usb0/ABooks/` on USB or `/data/AudioBookReader/Books/` locally (optional: `/download0/ABooks/`).

USB books are enumerated by the included `abookscan.elf` helper and need an already-running elfldr on TCP port 9021. The app transfers selected USB M4B files to `/data/AudioBookReader/Cache` before decoding; originals on USB remain untouched. If the helper cannot start, local books remain usable and the app shows its USB status. Encrypted FairPlay M4B audiobooks cannot be decoded. Embedded chapters require compatible MP4 chapter metadata. Audio output requires the FFmpeg/SDL2 libraries available in the PS5-DevBridge toolchain.

## Build

Run `.github/workflows/build-audiobook-reader.yml` via GitHub Actions; uses `PS5-DevBridge/templates/simple-calc`, native `ps5-homebrew-ui` OpenGL renderer, FFmpeg and SDL2 PacBrew dependencies. The workflow packages and validates executable, assets, metadata and release archive. Host smoke tests cover library scanning and progress persistence.

**Version:** v0.2.5  
**Title ID:** PPSA99784  
**Content ID:** UP9000-PPSA99784_00-AUDIOBOOKREADER1

License: GPL-3.0-or-later. Code derived from the GPL-compatible PS5 homebrew UI and development boilerplate.
