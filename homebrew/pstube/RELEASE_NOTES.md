# PSTUBE v0.7.1 — Fix 360p Only HD Streaming

- Resolve HD videos using the publicly accessible YouTube ANDROID_VR client instead of relying exclusively on legacy Android responses where the HD formats have no direct URLs.
- Prefer real H.264 720p/1080p video with separate AAC audio and valid verified HTTPS media URLs. Match the media client user-agent in native FFmpeg for both streams.
- Preserve Settings > Video quality (Auto, 360p, 720p and 1080p), bookmarks, seek, fullscreen, search and downloading.
- Add deterministic and live C++ tests asserting that Auto and 1080p produce actual HD video/audio URLs. The media-byte diagnostic verifies HTTP 206 and MP4 data, without exposing signed URLs.
- Use 360p only when selected or when a playable HD track pair is not available.

**Limitations:** HD still depends on unsigned URLs being supplied by YouTube; some titles restrict or omit them. 1080p software decoding can be too slow on PS5 homebrew. Legacy offline downloads still use the combined MP4 format (often 360p). A real PS5 hardware test is needed to confirm smooth HD playback.
