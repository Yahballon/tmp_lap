# PSTUBE v0.7.3 — Native PS5 Playback Beta

**PSTUBE by YAHBALLON** is an independent native C++ PS5 homebrew video search, download and playback client with a 1920×1080 OpenGL user interface.

## Features
- Search the public logged-out YouTube web page directly first, extract real video metadata using a native validated parser, then fall back to community Invidious instances. Both use verified HTTPS.
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
D-pad: navigate (keyboard/results/downloads). Cross: select, download/play video, pause/resume while playing. Circle: back / close player. Triangle: open search (download in video details). Square: favourite in library, backspace in keyboard. L1/R1: change tabs.

## Limits and compatibility
**Real console playback remains unverified** until tested in your environment.
- Playback is available only for downloaded, non-DRM combined MP4 streams with codecs supported by FFmpeg; separate adaptive video and audio, playlists, subtitles, high-res 4K, and live/HLS are not implemented.
- The public YouTube watch page may not expose any usable unsigned MP4 URL (signatureCipher), block bots or demand account login; the Invidious fallback can return HTTP 403. A full-screen error and retry affordance are displayed in that case. A working stream is not guaranteed. Browser-style YouTube account login and subscriptions are not implemented. This is not an official YouTube app. No Windows tools, companion servers, user-installed Python, account tokens or additional console installers are needed. Signed, login-required or PO-token-protected videos may still be unavailable.
- Downloads cap at 2 GiB; content-length validation for atomic completion; redirect destinations restricted to known public domains.
- Some endpoints may use chunked transfer encoding not supported by the MP4 downloader in this version.
- Native graphics plus SDL audio interoperability must be tested on physical PS5.
- Video is software-decoded; demanding 1080p formats may underperform.

## Version and build
Version 0.7.3. Title ID `PPSA99762`. Release tag `pstube-v0.7.3`. Dedicated workflow: `.github/workflows/build-pstube.yml`.
The workflow fetches PS5-DevBridge's native scaffold, stages PSTUBE C++ files and builds with pinned PacBrew ffmpeg, SDL2 and OpenSSL packages. GitHub Actions validates eboot and ZIP but is not hardware verification.

## Installation
Download `PPSA99762.zip` from the validated release, extract it to `/data/homebrew/PPSA99762/` and register with ShadowMountPlus or a compatible loader. Saved data: `/download0/pstube.txt` fallback `/data/pstube/state.txt`; videos in `/data/pstube/downloads/`. Do not delete these when updating.

GPL-3.0-or-later. Native renderer: https://github.com/blackbearreloaded/ps5-homebrew-ui. Media backend: FFmpeg and SDL2. Video metadata: community Invidious instances.

## Catalog troubleshooting
The Home page displays the last successful real catalog during temporary API failures and the exact provider/HTTP error. Search and Trending automatically try several community public servers. After updating, launch PSTUBE and wait for the first live fetch; press Triangle to search. Thumbnails download in three parallel workers and may arrive after titles. Network connectivity and correct PS5 date/time are essential for verified HTTPS. No fake videos are generated.

## Infinite scrolling and streaming (v0.7.3)
- Scrolling to the last few cards requests the next result page using YouTube's real search continuation token. Responses append to the existing grid, remove duplicates and display the number of loaded videos. The Invidious fallback also supports numbered pages. API pagination can stop when YouTube withholds or rejects continuation tokens.
- **Cross** begins buffered native HTTPS playback immediately on a compatible progressive MP4, without storing it first. **Square** while inside the player separately saves the video for offline use. Playback still uses FFmpeg/OpenGL and SDL. **Circle** aborts playback.
- YouTube signing restrictions and the PS5's availability of network-enabled FFmpeg still affect which streams can play; console testing remains necessary.

## Search results and navigation
- Navigate through the entire loaded search with D-Pad Up/Down or the left stick. The final partially filled page displays the preceding videos instead of showing only one or two tiles.
- Six videos per TV screen with a visible result range (for example 15-20 of 20) and a scroll position indicator.
- The existing v0.5.0 continuation engine fetches additional real YouTube search pages as you approach the last loaded results. No artificial 15-video search limit. Actual results depend on YouTube's response and network availability.

## Deep search pagination
As you scroll, PSTUBE follows YouTube's live continuation tokens across page 3 and beyond, supports multiple continuation response shapes, and de-duplicates videos. The available total depends on YouTube and is not necessarily every indexed result.

## Player controls (v0.7.3)
PSTUBE automatically searches **PS5 JAILBREAK NEWS** at launch. Search can be edited using the keyboard. During actual local or compatible HTTPS playback, use **D-Pad Left/Right** to rewind/fast-forward 10 seconds, **L1/R1** to rewind/fast-forward 30 seconds, **Options** to toggle true edge-to-edge full-screen video (aspect ratio preserved). **Cross** pauses/resumes, **Circle** returns to browsing, **Square** saves for offline use, and **Triangle** toggles Favourite. The seek operation happens on the FFmpeg worker thread; audio/codec queues are flushed on a successful seek. Remote servers must support seeking; for unsupported streams a diagnostic is displayed. A playback time/duration bar shows the current position and is briefly shown after button input during full-screen.

## Real HD playback settings (v0.7.3)
Open **Settings**, highlight **Video quality** and press Cross to cycle **Auto (up to 720p), 360p, 720p HD and 1080p Full HD**. D-Pad Up/Down changes the selected settings row; Cross on **Fallback search provider** still cycles providers. The quality choice is saved in the existing PSTUBE state file and preserved during app updates.

For HD, PSTUBE selects independent AVC/H.264 MP4 video and AAC/MP4 audio directly from the native YouTube Android player response. It opens both as separate verified HTTPS sources and synchronizes decoding through FFmpeg and SDL audio. The overlay reports **the decoded resolution**, not an assumed resolution. The app falls back to a lower available format if the selected one is unavailable; it never intentionally opens a video-only stream without its audio. The **Square offline download** currently saves the legacy combined MP4 format, which may be 360p even if HD streaming is selected.

Auto targets 720p to favor PS5 homebrew software decoding performance; selecting 1080p may have higher CPU, memory and network requirements. Actual available formats vary per video, and YouTube can restrict any track. The native in-app dual-track pipeline must still be verified on physical PS5 hardware.

## HD resolution source fix (v0.7.3)
The regular YouTube Android client can advertise 720p/1080p in `adaptiveFormats` without exposing any usable `url` for those tracks (SABR). PSTUBE now selects the ANDROID_VR client for **Auto / 720p / 1080p**, which was verified to serve direct HTTPS H.264 video (itag 136/137) and AAC audio (itag 140). The native FFmpeg player uses the matching user-agent for both tracks. 360p deliberately uses the original Android progressive path; if HD availability fails on a video, playback falls back to a compatible lower stream.

Real-network CI now requires the native Service to resolve *both* a 720p and 1080p H.264 stream plus separate AAC, not merely parse fixture data. Some YouTube videos may be restricted by login, proof tokens, DRM, or the provider's changed protocols. No resolution is fabricated; the overlay shows the decoder-reported pixel height.

## v0.7.3 HD preference fix
The PS5 video browser no longer overrides a selected 720p/1080p online stream with a previously downloaded 360p local MP4. The 360p cache is preferred only when 360p is explicitly selected or when the user plays a file from the Downloads tab. Auto checks online HD first. An explicit HD selection will display a missing-HD error if no HD video+AAC pair is available rather than silently playing a 360p fallback. The player shows both requested quality and actual decoded resolution. HTTPS dual-stream FFmpeg decoding is now an end-to-end test in GitHub Actions.

## Anonymous Guest HD Playback (v0.7.3)
PSTUBE requests temporary **guest visitorData** via `youtubei.googleapis.com/youtubei/v1/visitor_id` on the PS5 itself. It includes this value in VisionOS/Android VR player requests and retries once after `LOGIN_REQUIRED`. No external helper, account login or credentials are used. The token is kept in memory and not written to storage.

This improves compatibility with some anonymous public media but does **not** guarantee that YouTube will allow all HD videos to play without login. If the service requires account authentication or proof-of-origin, PSTUBE reports the restriction rather than mislabeling 360p as HD. A browser session and an unauthenticated native player are not equivalent.
