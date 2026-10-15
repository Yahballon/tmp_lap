# Ebook Reader v0.1.6 — CBR Comic Reader and Zoom

## New features
- Open **CBR** (RAR4 and RAR5) and **CBZ** (ZIP) image comics as native PS5 books.
- Read comic pages in correct numeric order, preserving image aspect ratio and showing first page as a cover on the bookshelf.
- **L2/R2** zoom out/in between 100% and 400%; pan with the **D-Pad** while zoomed; **L3/OPTIONS** resets.
- **L1/R1** switches pages. Cross next, Circle back, Triangle first and Square last.
- Zoom and panning are also supported in EPUB reading and PDF text preview.
- Existing USB helper discovers and transfers .CBR/.CBZ, supports archives up to 512 MiB.

## Installation
Install `PPSA99741.zip` over your current Ebook Reader from TESTBUILDS. Keep your existing saved progress and EPUB/PDF files. Same Title ID `PPSA99741` and helper ports 9021/9175.

## Known limitations
CBR depends on libarchive RAR4/RAR5 support; encrypted and multipart RAR archives are not supported. Images over 20 MiB or exceptionally large dimensions may be skipped for safety. GPU textures are capped to 2048px long edge. PDF remains an extractable-text preview, not a PDF graphics renderer. Actual console test is pending.
