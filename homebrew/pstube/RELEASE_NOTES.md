# PSTUBE v0.7.4 — Audio Language and Original Voice

- Add persistent audio-language preference in Settings: Original (default), English, German, Spanish, French, Japanese, Korean, Portuguese, Italian and Hindi.
- Select the desired AAC audio track from the YouTube player response instead of choosing whichever language has the highest bitrate.
- Prefer original language over an automatically dubbed locale-default track when audio-track metadata identifies the source track; fall back to the original when the requested translation is unavailable.
- Display the selected audio language in the player overlay, when the source reports metadata.
- Preserve the existing native 720p/1080p H.264 video decoder, separate AAC playback, seeking, fullscreen, search pagination and personal data.
- Add native C++ regression tests for English-original / German AI dub cases, explicit language selection, and missing-language fallback.

### Limitations
YouTube may omit or change per-track language/original metadata. In that case the app cannot reliably distinguish the original soundtrack and reports unavailable metadata rather than fabricating a language. The setting applies to the currently offered separate AAC streams; already-downloaded 360p combined MP4 files retain their existing audio. Real PS5 hardware testing is still required.

### Installation
Install `PPSA99762.zip` to `/data/homebrew/PPSA99762/`. Update your TESTBUILD catalogue. Do not delete `/data/pstube` or existing settings.
