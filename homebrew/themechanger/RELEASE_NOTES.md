# ThemeChanger v0.3.9 – Black Background Fix

- Analyzed the video logs and five actual MotionBGS HD downloads. Several files had H.264 High Level 5.0 metadata even at 1920x1080/30 fps; a comparison file uses 4.1.
- Normalizes a conservatively validated Full HD H.264 Level 5.0 `avcC`/SPS declaration to Level 4.2 **before** asking LiveShell to play it. Does not re-encode or modify actual video frames.
- Fix also runs when choosing an old downloaded background, so no redownload is needed to test it.
- Leaves videos already tagged Level 4.1/4.2 unchanged and rejects unsupported codecs/4K samples instead of silently producing a black background.
- Improved logs: `MP4_COMPAT` and `MP4_INCOMPATIBLE` with original/new level and resolution.
- Keeps the downloaded-only Backgrounds library, 9K+ searchable MotionBGS catalog, tag filters, gallery auto-refresh, and LiveShell engine integration untouched.
- MP4-level compatibility validated by GitHub-hosted FFprobe and file-preservation tests. **Visible PS5 playback must be verified on console.**
