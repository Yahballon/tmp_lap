# Ebook Reader v0.1.7 — Full-Cover Bookshelf & Zoom Page Reset

## Fixed
- **Start at the top on every new page, even when zoomed:** Previous/next-page navigation now clears the previous pan position while retaining magnification. The scale origin moves from the viewport centre to the first reading row or top of the comic viewport. Landscape comic art gets an automatic top offset, so the new page is visible from its beginning.
- **No more overlapping books:** The bookshelf uses a tested five-column, two-row grid (10 covers per screen) with 65px separation between rows instead of placing 268px-tall books only 235px apart. Wooden shelves are redrawn to align with the bottom of each row. Selection and paging continue to work with long book lists.
- **Full-bleed real covers:** The image is rendered across the complete visible book front using centred UV cropping. No more stuck-on thumbnail/label appearance. Placeholder titles remain only for books without embedded artwork, and favourite/progress marks are kept unobtrusive.

## Installation
Install `PPSA99741.zip` through TESTBUILDS. Title ID stays `PPSA99741`; existing library and saved state remain compatible.

## Controls
L1/R1 previous/next page; L2/R2 zoom out/in; D-pad pan while zoomed; L3/OPTIONS reset zoom; X next page; Circle bookshelf. Same CBR/CBZ, EPUB images, USB-helper functionality.

## Validation and limitations
GitHub Actions checks page layout and UV calculations, zoom top alignment and compiles the complete native PS5 package. Physical-console validation is still needed. PDF remains an extractable-text preview, not a full graphics PDF renderer.
