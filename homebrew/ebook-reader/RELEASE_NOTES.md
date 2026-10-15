# Ebook Reader v0.2.5 — Progress Bar and Comic Covers

## New features
- Asynchronous book opening: the rendering loop stays active during USB transfers and archive parsing.
- Large modal shows exact USB download progress in MiB and percentage, followed by the document preparation stage. Circle requests download cancellation.
- CBR/CBZ comic covers load from the first image, in background. EPUB thumbnails are prioritized.
- Persistent thumbnails stored in /data/EbookReader/Covers, separately from large disposable comic archives.
- A small progress bar on the bookshelf shows comic-cover download progress.

## Compatibility
The working /data/EbookReader/Cache from v0.2.4 is unchanged. Local books in /data/EbookReader/Books, favourites, reading progress, existing EPUB/PDF/CBR/CBZ support, title ID PPSA99741 and USB bridge port 9176 are preserved.

## Validation
CI includes a real 48 MiB interrupted comic transfer and checks monotonic byte progress, cover file persistence, nonblocking book-opening code, native PS5 compilation and the complete ZIP.

## Installation
Install PPSA99741.zip via TESTBUILDS. Existing books and save data need not be removed. An uncached USB comic may take time to acquire its cover initially; subsequent sessions reuse the small saved thumbnail. Hardware testing on PS5 remains necessary.
