# PS5DreamcastX v0.1.5

## Emergency PS5 CoverFlow renderer correction

- Replaced v0.1.4's white/blank cases and misplaced giant images with
  v0.1.3's PS5-tested axis-aligned imageFill texture path.
- Kept larger white Dreamcast-style cases and nearly full-face artwork.
- Kept centered title and consistent left/right case scaling.
- Temporarily disabled buggy perspective texture quads and reflections.
- Added host tests that reject unsafe draw-list primitives and check
  actual cover placement, bounds and animated focus changes.
- No emulator, USB, BIOS, save, or configuration changes.

Native Title ID `PPSA99765`. Download `PPSA99765.zip`.
