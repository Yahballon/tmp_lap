# Ebook Reader v0.1.0
Native PS5 bookshelf inspired by the uploaded wooden-themed React eBook Reader, built on blackbearreloaded/ps5-homebrew-ui.

## Features
- Wooden shelf with book tiles, list mode, collections and favourites.
- Local and USB library scanning, USB import to /data/ebooks.
- EPUB chapter order, XHTML text reflow, two-column reading.
- PDF plain extractable-text preview, not a PDF graphics engine.
- Saved favourites, reading progress, and metadata with atomic file replacement.

## DualSense
D-pad navigates; Cross opens/next page; Circle back/exit; L1/R1 switches library collection or reader page; L2/R2 jumps 10 pages in reader; R2 toggles list/grid in library; Square favourites in library and last page in reader; Triangle imports via USB or jumps to first page; Options rescans library; L3 asks to delete a locally imported file.

## Files and limitations
Put .epub or .pdf books in /data/ebooks, or /mnt/usb0/Books. Import copies the source to /data/ebooks. 128 MiB file cap and 12 MiB single decompressed EPUB entry cap. DRM, scanned PDFs, EPUB graphics, complex EPUB formatting, embedded PDF font encoding, and full PDF rendering are not supported. The EPUB reflow is text-only. This is a native reimplementation, NOT a pixel-exact 1:1 browser port. On-device PS5 testing is outstanding.

## Build and install
The dedicated build-ebook-reader.yml workflow uses PS5-DevBridge, zlib and a PS5 OpenGL SDK. After a successful verified main-branch build, the ZIP should be published under tag ebook-reader-v0.1.0. Download PPSA99741.zip, extract PPSA99741 to /data/homebrew and register with a compatible PS5 homebrew launcher.

Source license: GPL-3.0-or-later. Respect dependency licenses.
