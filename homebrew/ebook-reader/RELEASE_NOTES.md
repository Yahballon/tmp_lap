# Ebook Reader v0.1.8 — Complete Zoom Scrolling & Single/Double Pages

## Fix: Read all the way to the bottom while zoomed
- Replace approximate fixed zoom/pan clamps with bounds derived from the actual image width, height, displayed aspect ratio, current zoom level and visible viewport.
- At any zoom from 100% to 400%, pressing Down can now reach the very bottom of portrait, landscape, square and unusual-aspect-ratio comic pages. EPUB and PDF text preview pages also scroll fully.
- Maintain the v0.1.7 behaviour: a new page begins at its top, with the selected zoom level retained.

## Added: Square switches EPUB/PDF page mode
- Press **SQUARE** in an EPUB or PDF book to toggle **Double Page ↔ Single Page**.
- Double-page shows two consecutive 17-line paper columns; single-page shows one centred 17-line paper sheet. The current reading position is converted to the new layout, and the image/scroll position is refreshed without reopening the book.
- Progress saved for both modes, using position-normalized page counts; book/USB files and existing saved reading progress are preserved.
- CBR/CBZ comics remain full-screen single-page; SQUARE continues to jump to the last page when reading comics.

## Controls
L1/R1 previous/next page; L2/R2 zoom out/in; D-Pad pan while zoomed; L3/OPTIONS reset zoom; SQUARE one/two page switch in EPUB/PDF; Cross next; Circle bookshelf.

## Validation & installation
Build and ZIP validation, plus geometry tests covering complete vertical/horizontal pan travel at 125%, 200%, 300%, 400%, multi-aspect comics and single/double text page position conversion, are required for release.
Install PPSA99741.zip from TESTBUILDS. Title ID PPSA99741 is unchanged.
Physical PS5 testing remains outstanding. PDF is still a text-only preview, not full graphical PDF rendering.
