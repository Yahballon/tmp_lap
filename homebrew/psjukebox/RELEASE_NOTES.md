# PSJukebox v0.1.6 (TESTBUILD)

- Drop `.album` ZIPs directly in Music/ with internal empty `Tupac.artist` (or `<Artist>.artist`) marker. No manual artist folders needed.
- Keep Music/Artist/*.album and Music/Artist/Album/ folders fully supported.
- PS5 read-only USB helper can now enumerate Music-root .album files.
- Album-cover **Down** opens the track list directly; X starts playback as before.
- Native SMB2/3 read-only share support via `/data/PSJukebox/smb.ini` (server, share, optional path, username, password, domain).
- Browse NAS/Windows artist folders, stream artwork/track files into the local player cache on demand, and import packed .album packages from SMB.
- Preserve existing artist themes, Favorites, coverflow, FFmpeg decoding and other controller mappings.

Limitations: SMB requires network access/permissions; one configured share; disk cache for audio and archive extraction; password stored in plaintext in user-managed smb.ini. Tested by CI host compilation/native PS5 package checks, not yet hardware-verified.
