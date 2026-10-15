# PSTUBE v0.7.3 — Anonymous Guest HD Session (Experimental)

- Retrieve a temporary, unauthenticated YouTube visitor context directly on PS5 from the public visitor_id endpoint.
- Attach guest visitorData to both VisionOS and Android VR player requests for 720p/1080p, and refresh that identity once when YouTube returns LOGIN_REQUIRED.
- Keep the guest identity in RAM only; do not request Google credentials, use browser cookies, install Windows helpers, or persist login secrets.
- Improve errors to distinguish an HD stream being unavailable from an explicit rejection of anonymous playback.
- Preserve 360p fallback, existing player, seeking, fullscreen, favourites and settings.

## Important limitation
YouTube may still return LOGIN_REQUIRED or demand playback attestation even for videos that play in an ordinary browser. In live CI tests, multiple publicly indexed PS5 jailbreak news videos remained login-restricted across guest client profiles, so this update is **not a guaranteed bypass** of YouTube's anonymous playback restrictions. True 720p/1080p requires YouTube to return accessible HD and AAC URLs. No DRM or authentication restrictions are circumvented.

To test: Settings → Video quality → 720p; open a known public video. If playback is refused, send the video's URL and the exact on-screen error.
