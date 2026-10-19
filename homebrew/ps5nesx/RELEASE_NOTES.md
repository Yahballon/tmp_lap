# NESYX v1.0.24 by YAHBALLON

- Rebrand PS5NES/PS5NESX to NESYX in the PS5 menu, native CoverFlow, web settings and release display names.
- Show "by YAHBALLON | v1.0.24" under NESYX in the main window header (version generated from app.json).
- Keep game titles free of the redundant console-system subtitle and retain the exact 1024x1536 NO COVER PNG with a full-size shared texture.
- Preserve title ID PPSA99204, install/data locations, PS5NESX source slug and legacy release-tag prefix for existing TESTBUILDS installations.
- Real downloaded boxart still replaces the fallback, tiger header icon retained.

# PS5NESX v1.0.23 by YAHBALLON

- Embed the exact high-resolution 1024 x 1536 PNG supplied for NO COVER in the PS5 binary (full-size, no JPEG compression or black letterboxing).
- Remove the redundant "Nintendo Entertainment System" subtitle below the selected game.
- Keep a single shared 1536px GPU fallback for Porpoise coverflow; preserve bounded legacy fallback memory use and automatic real-artwork overrides.
- Preserve tiger logo, cached cover downloading and hidden Recently Browsed dock.

# PS5NESX v1.0.22 by YAHBALLON

- Use the supplied neon NO COVER artwork (embedded, JPEG) for games without a valid local, cached or downloadable cover (also in the original Porpoise CoverFlow).
- Replace the Dolphin mark in the header with a custom blue tiger emblem.
- Remove the Recently Browsed dock and its selection-history bookkeeping.
- Preserve real cover download, refresh, caching and the regular fallback view.

# PS5NES v1.0.0.9

(AI-assisted) Fixes silent omission of readable internal ROMs under metadata restrictions, adds internal ROM folder aliases, retains bounded ROM/cache reads and displays the selected ROM path below the title. Existing local/USB/NFS cache behavior remains covered by regression tests. Native metadata: 01.000.009; title PPSA99204.
