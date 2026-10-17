# PSJukebox v0.1.0 — PS5 TESTBUILD

Native PlayStation 5 offline homebrew test build by YAHBALLON. Hardware test: not performed.

- Artist-specific logo.png and theme.ini, full-screen changing album background images
- Five-cover album carousel and native FFmpeg/SDL2 player with DualSense controls
- Music layout: Music/Artist/Album/music files + cover.png/jpg + background.png/jpg
- Place the PPSA99947 folder from PPSA99947.zip into /data/homebrew with a compatible loader
- Setup: https://github.com/Yahballon/tmp_lap/tree/main/homebrew/psjukebox

Limitations: Direct /data and USB access depends on loader permissions; no privileged helper yet. Some codecs and DRM files are unsupported. Large artwork may load slowly. No copyrighted album images or audio are distributed.

This is a regular release rather than GitHub prerelease solely for compatibility with TESTBUILDS schema-2 catalog discovery; **TESTBUILD** remains in the title.
