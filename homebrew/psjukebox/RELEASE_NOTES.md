# PSJukebox v0.1.17 — Unicode + New HUD Design

- Fix missing German umlauts and accented letters using a checked DejaVu international SDF fallback atlas, alongside Inter
- Support verified extended Latin, Greek and Cyrillic glyphs, including Eastern European and Ukrainian names
- Correct UTF-8 title truncation to preserve complete multibyte characters
- Replace plain header with a glass HUD including the neon app icon, focus pills, indexing status and clock / by YAHBALLON / build version
- Add artist / track-count context bar and album statistics cards
- Enhance coverflow focus, layered shadows and glow while preserving album spacing
- Add Now Playing glass player, progress bar, subtle animated equalizer
- Harmonize Tracklist, Artists, Favorites and Settings with readable frosted glass panels
- Design cues follow ps5-homebrew-ui Field HUD, Aurora Shelf and Now Playing; no framework replacement
- Retains native PS5 playback, .album, SMB, favorites, fast library cache and all gamepad mappings

Limits: Arabic shaping / RTL and full CJK ideograph coverage are not implemented. Native build tests pass independently of real-console validation.
