# PSJukebox v0.1.5 — .album package support (TESTBUILD)

New custom packed album support: drop a ZIP renamed `.album` straight into any Artist folder, next to existing album folders.

- Detect .album from local folder scan and the PS5 read-only USB helper
- Stage and safely unpack ZIP members into an isolated PSJukebox cache
- Support MP3/FLAC/M4A/AAC/WAV/OGG/OPUS/WMA plus cover.png/jpg/jpeg and background.png/jpg/jpeg
- Display the packed file as an album with cover/background; play extracted tracks with existing FFmpeg player
- Ignore incidental ZIP/XML/SQLite metadata; strip an optional single wrapping folder
- Reject path traversal, symlink, encrypted, ZIP64 and oversized/unsupported archive entries
- Keep existing Artist/Album music folders, favorites, smooth coverflow and DualSense controls unchanged
- Host regression test covers deflate + stored compression, wrapped song, cover and background

Supported archive size up to 1 GiB; limit 2 GiB expanded. Import needs free PS5 local storage for archive and extraction. Real-console validation required.
