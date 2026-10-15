# PSTUBE v0.4.1 — Native PS5 Playback Beta

**PSTUBE by YAHBALLON** is an independent native C++ PS5 homebrew video search, download and playback client with a 1920×1080 OpenGL user interface.

## Features
- Search YouTube with native paginated continuation API calls over verified HTTPS. Scroll down to automatically fetch more 20-video pages, de-duplicate IDs, and retain up to 400 results per query; Invidious remains a fallback if the first search fails.
- Live Trending search, automatic failover between currently documented Invidious instances, and manual provider selection in Settings.
- Three native workers fetch actual 16:9 JPEG thumbnails using browser-compatible HTTPS headers. The cache tests write access under /data/pstube/thumbs and falls back to /download0/pstube/thumbs when necessary. Settings shows successes, failures and the latest precise error.
- Persistent favourites and watch history, plus a cache of the last successful live video catalog for offline startup.
- Left-stick navigation and Options shortcut to Settings.
- DualSense-driven onscreen keyboard and selection navigation.
- Persistent favourites and saved downloads stored outside app folder.
- **Cross opens the player immediately**. Already downloaded files play right away. Otherwise a full-screen loading/progress/error player resolves a compatible combined MP4 stream from the native Android Innertube endpoint, public watch-page metadata or Invidious fallback, downloads it atomically, then starts playback. Triangle toggles a favourite.
- Locally play downloaded video with FFmpeg 7 software decoding, OpenGL RGBA frames and SDL2 queued audio.
- Downloads tab shows progress and allows playback for completed files.

## Controls
D-pad: navigate (keyboard/results/downloads), continuously scroll through all loaded results; the next page loads automatically near the end. Cross: select, download/play video, pause/resume while playing. Circle: back / close player. Triangle: open search (download in video details). Square: favourite in library, backspace in keyboard. L1/R1: change tabs.

## Limits and compatibility
**Real console playback remains unverified** until tested in your environment.
- Playback is available only for downloaded, non-DRM combined MP4 streams with codecs supported by FFmpeg; separate adaptive video and audio, playlists, subtitles, high-res 4K, and live/HLS are not implemented.
- The public YouTube watch page may not expose any usable unsigned MP4 URL (signatureCipher), block bots or demand account login; the Invidious fallback can return HTTP 403. A full-screen error and retry affordance are displayed in that case. A working stream is not guaranteed. Browser-style YouTube account login and subscriptions are not implemented. This is not an official YouTube app. No Windows tools, companion servers, user-installed Python, account tokens or additional console installers are needed. Signed, login-required or PO-token-protected videos may still be unavailable.
- Downloads cap at 2 GiB; content-length validation for atomic completion; redirect destinations restricted to known public domains.
- Some endpoints may use chunked transfer encoding not supported by the MP4 downloader in this version.
- Native graphics plus SDL audio interoperability must be tested on physical PS5.
- Video is software-decoded; demanding 1080p formats may underperform.

## Version and build
Version 0.4.1. Title ID `PPSA99762`. Release tag `pstube-v0.4.1`. Dedicated workflow: `.github/workflows/build-pstube.yml`.
The workflow fetches PS5-DevBridge's native scaffold, stages PSTUBE C++ files and builds with pinned PacBrew ffmpeg, SDL2 and OpenSSL packages. GitHub Actions validates eboot and ZIP but is not hardware verification.

## Installation
Download `PPSA99762.zip` from the validated release, extract it to `/data/homebrew/PPSA99762/` and register with ShadowMountPlus or a compatible loader. Saved data: `/download0/pstube.txt` fallback `/data/pstube/state.txt`; videos in `/data/pstube/downloads/`. Do not delete these when updating.

GPL-3.0-or-later. Native renderer: https://github.com/blackbearreloaded/ps5-homebrew-ui. Media backend: FFmpeg and SDL2. Video metadata: community Invidious instances.

## Catalog troubleshooting
The Home page displays the last successful real catalog during temporary API failures and the exact provider/HTTP error. Search and Trending automatically try several community public servers. After updating, launch PSTUBE and wait for the first live fetch; press Triangle to search. Thumbnails download in three parallel workers and may arrive after titles. Network connectivity and correct PS5 date/time are essential for verified HTTPS. No fake videos are generated.
