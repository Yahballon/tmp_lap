# Audiobook Reader v0.2.4

## Fixed
- The **current chapter updates automatically during playback** as the player clock crosses embedded M4B chapter boundaries. No need to stop, return to the bookshelf, or reopen the audiobook.
- Live chapter number and chapter title refresh after seeking forward/backward (D-Pad, L2/R2), chapter skips (L1/R1), resume, and ordinary continuous listening.
- The chapter list independently marks the currently playing entry while keeping manual up/down selection stable, and switches back to live selection when the user stops navigating or opens the list anew.
- Chapterless M4B files clearly display "No embedded chapters" instead of "Chapter 1 / 0".
- Fixed bookshelf Cross selection to open the highlighted book when multiple books are present, and the Now Playing panel to show the actual open title even with active library filters.

## Existing features retained
- USB M4B from `/mnt/usb0/ABooks`; local M4B from `/data/AudioBookReader/Books` via dedicated ELF helper on TCP 9179 with elfldr TCP 9021.
- Embedded M4B cover art, raw MP4 covr fallback, companion JPEG/PNG files and persistent cover cache.
- Progress saved under `/data/AudioBookReader/progress.tsv`, diagnostics at `/data/AudioBookReader/audiobook-reader.log`.
- Title ID: `PPSA99784`. Preserve `/data/AudioBookReader` during update.

## Verification
Host regression test covers boundary timestamps, skips, reverse seeks, missing chapters and malformed timestamps. PS5 native build and ZIP packaging run in GitHub Actions. Hardware behavior still requires a console test.
