# Ebook Reader v0.2.1 — CBR/RAR compatibility fix

## Fixed
- CBR and CBZ books stored locally now stay in the **Comics** collection rather than being incorrectly replaced by Fiction.
- More reliable RAR4/RAR5 image entry recognition, including Windows-style backslash paths, optional ./ path prefixes and archives with incomplete file type metadata.
- Added decompressed JPEG/PNG/GIF validation and informative errors for unreadable or encrypted comic images.
- Host CI verifies complete RAR4 and RAR5 file decompression instead of checking just archive headers.

## Preserved
Existing EPUB/PDF full-width and single/double pages, reading progress, cover art, DualSense zoom/pan, CBR/CBZ reader, and read-only USB helper.

## Install
Download `PPSA99741.zip` and install via TESTBUILDS. CBR/CBZ files may reside in `/mnt/usb0/Books/` or the app-local Books library. For USB access keep compatible elfldr running on port 9021. Press OPTIONS to rescan and choose the Comics collection.

## Limitations
Password-protected or multi-part RAR archives, exotic/unsupported RAR filters and non-JPEG/PNG/GIF images are not supported. Tested in CI; actual PS5 hardware test is still required.
