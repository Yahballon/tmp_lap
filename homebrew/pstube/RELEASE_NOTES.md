# PSTUBE v0.4.0 — Native Stream Resolver

- New: Native HTTPS Innertube Android player support in the PS5 C++ application. No Windows PC, bridge server or companion application is required.
- Verify available combined MP4 (H.264/AAC) URLs from YouTube's Android client, which can return direct URLs even when the web player returns HTTP 302 or signatureCipher.
- Secure TLS verification, HTTP POST and safe YouTube redirects; preserve existing YouTube web and Invidious fallback.
- Correctly decode JSON Unicode escape sequences in media URLs (including encoded query ampersands).
- Keep automatic playback after a verified downloaded MP4 and existing TV UI, favourites, downloads, thumbnails and self-test.
- Add real network tests against the public Android player API and verify actual MP4 magic bytes in CI.

**Limitations:** YouTube decides which videos are playable unauthenticated. Some videos require login, proof-of-origin tokens, a JavaScript signature solver or only offer adaptive streams. PSTUBE explicitly reports these failures; it does not defeat DRM, bypass authentication, or guarantee playback of every video. Full-download-before-play and physical PS5 audio/video testing remain necessary.

Install the verified PPSA99762.zip into /data/homebrew/PPSA99762 and refresh TESTBUILDS. Saved data under /data/pstube and /download0/pstube is preserved.
