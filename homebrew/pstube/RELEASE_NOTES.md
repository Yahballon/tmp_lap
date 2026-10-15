# PSTUBE v0.3.0 — Player and Thumbnail Reliability Update

- Fix reported empty thumbnails: browser-compatible request headers, writable cache directory detection (/data with /download0 fallback), JPEG validation, three parallel image workers, and on-console thumbnail diagnostic counters.
- Replace the extra info-page step: **Cross on a video now opens the full-screen video player immediately**.
- Player automatically opens completed local videos or displays actual MP4-resolution and download progress, and starts after a completed verified download.
- Resolve eligible progressive MP4 URLs directly from YouTube watch-page player metadata, with Invidious as fallback.
- Display precise signed-stream, bot-check, HTTP and provider errors on the player; Cross retries failed downloads.
- Larger 16:9 thumbnails, richer TV-sized cards and inline status.

**Known limitations**: YouTube commonly exposes only signatureCipher or separate adaptive audio/video formats, and may reject unauthenticated playback. No YouTube signing/JavaScript runtime is implemented. For those videos, PSTUBE shows an error rather than pretending playback works. FFmpeg decoding and SDL audio still require a real PS5 hardware test. It is not an official YouTube client.

Download PPSA99762.zip and replace /data/homebrew/PPSA99762. Keep /data/pstube and /download0/pstube to retain saved data.
