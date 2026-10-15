# Audiobook Reader v0.2.5 — HD PS5 App Icon

Replaces the blurry 48px application icon with the user-approved 512×512 neon bookshelf and headphones artwork. Both the PS5 shell `sce_sys/icon0.png` and app asset use the new high-resolution icon. The CI build validates that icon pixels in the release package match the repository source image exactly.

All audiobook playback features, live chapter updates, local/USB library access, covers, favorites and listening progress are unchanged. Install `PPSA99784.zip` over the old app folder. Preserve `/data/AudioBookReader` and refresh/restart the homebrew launcher if the PS5 caches the previous thumbnail.
