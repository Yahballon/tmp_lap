# PSJukebox v0.1.2 — PS5 TESTBUILD

Multi-artist and DualSense UI fixes:

- Keep artist folders visible regardless of missing/empty album folders, e.g. Tupac alongside wu-tang.
- Keep empty album jackets visible instead of silently dropping them; detect songs directly in artist folders as Singles.
- Merge local and helper scan results by artist and album.
- Both DualSense analog sticks and D-pad support cover browsing with controlled repeat; current selected jacket always remains centered in the circular carousel.
- Bottom hints replaced with colored controller button symbols; removed Multi Album Player from the artist header.
- Extended host regression tests for directory-only artists, zero-track albums, artist-root music, and analog navigation.

No change to USB folder layout: Music/Artist/Album/{cover.jpg, background.jpg, songs}, Artist/logo.png.
If another artist has no playable songs, it is still visible and the app logs its album and track counts.
Requires running compatible elfldr on port 9021 for restricted USB access. Physical PS5 hardware verification remains pending.
