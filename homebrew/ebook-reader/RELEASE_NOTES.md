# Ebook Reader v0.1.9 — Single-page zoom clipping and readable footer

## Fixed
- Magnified EPUB/PDF text no longer bleeds beyond the cream parchment sheet into the brown side gutters.
- A physical paper scissor is applied before the CPU zoom transformation.
- At 125–400% zoom the first character stays aligned with the page's natural left margin, while the full document remains pannable.
- New pages retain magnification but reset scrolling to their top-left reading origin.
- Page number, Single/Double Page indicator and zoom percent are displayed in a dark footer outside the text.
- Preserves existing USB helper, comic zoom, EPUB images, favourites and reading progress.

## Validation
Regression tests now cover clipping boundaries, reachability and text alignment at 100%, 125%, 150%, 175%, 200%, 250%, 300%, 350% and 400%.

## Installation
Install PPSA99741.zip via TESTBUILDS with Title ID PPSA99741. The usual PDF text-preview limitation remains. Physical PS5 confirmation is pending.
