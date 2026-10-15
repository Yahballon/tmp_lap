# PSTUBE v0.7.2 — True HD Playback Fix

- Correct cached-file playback precedence: selecting 720p or 1080p now uses the native HD resolver even if an earlier 360p MP4 is already downloaded.
- For explicitly selected HD, do not silently downgrade to the old 360p Android/Web/Invidious stream; display the real failure instead.
- Show requested quality alongside actual decoded frame height in playback.
- Add an end-to-end native C++/FFmpeg test that must **decode actual 720p and 1080p HTTPS video**, not merely parse URLs, with separate AAC input.
- Preserve existing search, favourites, saved downloads, controller shortcuts and user preferences.

HD remains dependent on available YouTube tracks and hardware decoding throughput. Streaming HD is separate from the Downloads library; existing 360p offline files are preserved. No companion PC tools needed.
