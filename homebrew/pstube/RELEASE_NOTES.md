# PSTUBE v0.2.0 — Playback Beta

PSTUBE by YAHBALLON adds a real software decode pipeline and MP4 download manager.

- Resolve combined MP4 streams through the configured public Invidious API.
- TLS-verified download to /data/pstube/downloads with atomic .part completion and progress.
- Decode completed MP4 video with FFmpeg, render with native OpenGL and queue audio through SDL2.
- Full DualSense play/pause/back controls, Downloads list, working Trending endpoint and provider switching.
- Saved favourites, video titles, playback history and completed files survive application updates.
- Asynchronous caching and rendering of real video thumbnails.
- Left stick navigation and Options Settings shortcut.

**Limitations:** Playback requires first downloading a compatible combined MP4 stream; high-resolution separate adaptive audio/video formats, thumbnails, YouTube login, background playback and subscriptions are not supported. Invidious APIs and signed media URLs may be unavailable or restricted. First real-hardware playback test pending.

Install: extract PPSA99762.zip to /data/homebrew, replace the previous version, and re-register in the loader. Source and GPL notice in the repository.
