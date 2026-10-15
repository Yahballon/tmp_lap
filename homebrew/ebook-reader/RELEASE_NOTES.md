# Ebook Reader v0.2.0 — Full-width Single Page Reading

## Improved
- **Full-width single-page canvas**: the Single Page mode selected with Square now fills the entire parchment within the native PS5 book frame, with no narrow centred sheet or dark blank side gutters.
- **Real text reflow**: EPUB and PDF text is rewrapped from the conservative double-column source lines to ~89-column lines suitable for the full-width 1484px reading surface. Text no longer occupies only the left half after making the paper wider.
- **Images in Single Page**: embedded EPUB JPEG/PNG illustrations retain their positions in the new layout and have the full available drawing width.
- **Smart view-mode transition**: switching Single/Double keeps the nearest original text position. Existing dual-page pagination, page navigation, zoom and reading progress remain available.
- **Zoom and pan**: 100–400% zoom clips to the complete book canvas and can pan across text to the right edge and the bottom; a new page still starts at its reading origin.

## Installation
Update `PPSA99741.zip` through your TESTBUILDS app. Title ID remains `PPSA99741`; your saved data, USB helper, EPUB cover art, PDF text preview and CBR/CBZ support remain intact.

## Validation
The independent PS5 CI runs new full-width text/image layout tests, existing geometric zoom tests, EPUB/RAR/USB regression tests, then native compilation and ZIP verification before release.

## Known limitations
PDF rendering is still a text-only preview (not full graphical PDF layout). Very complex EPUB CSS is not reproduced. Physical PS5 validation is required.
