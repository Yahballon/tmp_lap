# PSJukebox v0.1.18 — Native Screenshot Dashboard

- Rebuild the UI composition to closely match the supplied neon PSJukebox dashboard screenshot in **native C++ DrawList**, not as a nonfunctional flattened background
- New full glass header, neon PSJukebox app icon, split white/gold wordmark and subtitle, five tabs with custom vector glyphs and highlighted gold focus state
- Reposition library-ready status, 24-hour clock and **by YAHBALLON · v0.1.18**
- Tall artist hero panel with current artist name, actual artist/song totals, and live ready/scanning status
- Screenshot-style contextual glass cards ('NOW BROWSING', 'ALBUM') behind five real album covers
- Preserve smoothly animated coverflow and reflections, with selected cover higher, centered and highlighted in gold; four neighboring covers remain interactive
- Full-width Now Playing with the currently playing album art, real title, artist/album details, real playback seek progress and timestamps plus animated gold spectrum
- Align controller footer with nine dark glass chips and color-matched DualSense key icons, and draw carousel direction arrows
- Retain existing album/artist themes, Unicode accents and umlauts, .album reader, SMB, local/USB, playback behavior, fast-start catalog, favorites, and user music
- Add repeatable native screen-geometry tests and renderer integration checks

Tested in GitHub Actions (native PS5 build and package validation). Exact appearance and performance require user confirmation on PS5 hardware.
