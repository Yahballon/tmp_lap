# PSJukebox v0.1.10 — Fix .album shows 0 tracks on real PS5

From actual hardware v0.1.9 logs:
`ALBUM_OK wutang/wu-tang-forever tracks=0` and
`UNAVAILABLE /data/PSJukebox/AlbumCache/.../Music errno=1`.

The custom ZIP extraction completed successfully, but the post-extraction
library scan used `opendir` on a sandbox-protected PS5 cache directory.
Because `opendir` was denied, the imported album had zero songs and no art.

This release synthesizes the album's media catalog **directly from its validated
ZIP central directory** instead of scanning cached directories. Music titles,
file sizes, paths, cover and background artwork are indexed even when directory
enumeration is impossible. Cache reuse also regenerates the index instead of
returning early without tracks.

Adds a dedicated CI regression that denies `opendir` on AlbumCache with
EPERM, verifies two playable tracks + cover/background, and validates
repeat import on an existing archive-ready cache. Retains v0.1.9 heap ZIP
buffers, coverflow layout (no folder name above cover), favorites, SMB,
and controller mappings. No media source files are modified.

Actual playback needs user confirmation on PS5 hardware.
