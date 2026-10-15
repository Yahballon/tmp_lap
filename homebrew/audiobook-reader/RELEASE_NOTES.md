# Audiobook Reader v0.2.3

## Fixed
- Local Books at `/data/AudioBookReader/Books` can be catalogued through the included read-only ELF helper when the PS5 app process reports `errno=1 (EPERM)` on `opendir`. Selected books are copied to the existing writable audiobook cache, preserving originals.
- Separate `LISTLOCAL`, `FETCHLOCAL` and resume commands; helper port 9179 avoids stale resident processes.
- Album art fallback chain: native FFmpeg embedded image, direct iTunes MP4 `covr` parser and small adjacent JPEG/PNG covers via the helper. No full USB audiobook transfer is required just to read a companion cover.
- Per-title cover cache and existing aluminum-shelf UI retained.
- Additional diagnostics for local helper catalog failures and companion cover transfers.

## File layout
USB: `/mnt/usb0/ABooks`; local: `/data/AudioBookReader/Books`.
Optional companion images next to the audiobook: `cover.jpg`, `folder.jpg`, `front.jpg`, `cover.png`, `folder.png` or `NameOfBook.jpg/png`.
Cover cache: `/data/AudioBookReader/Covers`; playable cache: `/data/AudioBookReader/Cache`; logs: `/data/AudioBookReader/audiobook-reader.log`.

## Installation and limitations
Unzip `PPSA99784.zip` under `/data/homebrew/` and register with a compatible loader. Existing app data is retained. An active elfldr on port 9021 is required for local-helper fallback and USB content. When no embedded or companion artwork exists, a generated title placeholder remains; the app does not invent a published book cover. Hardware verification is still needed.
