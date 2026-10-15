# PSTUBE v0.7.0 — Native HD Quality Settings

- Add persistent quality options: **Auto (up to 720p), 360p, 720p HD, 1080p Full HD**.
- Select independently hosted AVC/H.264 MP4 video and AAC/MP4 audio streams at the requested resolution when offered by YouTube's Android player response.
- Feed both sources to the native FFmpeg/SDL software decoder and synchronize audio with video.
- Preserve fullscreen, pause, ±10s/±30s seeking, favorites, downloads, search pagination, and default PS5 JAILBREAK NEWS.
- Display actual decoded video resolution in the playback overlay.
- Retain a working lower-resolution combined MP4 fallback when no suitable HD + audio pair is available.
- CI tests verify format selection, 720p playback with separate AAC audio and seeking, followed by PS5 package validation.

**Known limitations:** Not every video offers playable unsigned H.264/AAC tracks. 1080p software decoding on PS5 homebrew may drop frames; use Auto/720p if necessary. Offline downloads still use combined MP4 (typically 360p). Hardware HD performance has not been validated.
