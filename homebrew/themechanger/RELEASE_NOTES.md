# ThemeChanger v0.3.10 - Smooth Video & Persistent Downloads

- **True on-console H.264 conversion:** Native FFmpeg/libx264 automatically decodes MotionBGS streams with SPS `max_num_ref_frames > 4` and re-encodes a Full HD 30 FPS H.264 Baseline MP4 with one reference frame, no B frames, proper Level 4.1. Fixes the underlying decoder limits instead of changing only level metadata. Converted videos are fully self-contained and replace the original only after a complete successful encode.
- **Works on old downloads:** Selecting an existing MP4 that still has eight reference frames queues the same conversion on a worker thread. No redownload or Windows tool is required.
- **Real progress:** ThemeChanger reports "Optimizing ... for smooth PS5 playback" while the worker runs; conversion is one-time and may take longer than downloading.
- **Saved across app/console restarts:** Downloads are mirrored atomically into `/data/ThemeChanger/wallpapers` and restored to LiveShell's working video directory on the next ThemeChanger launch. Existing saved MP4s are migrated on first run. A persistent copy is never overwritten by partially downloaded data.
- Added diagnostic events `VIDEO_OPTIMIZE_START`, `VIDEO_OPTIMIZE_DONE`, `VIDEO_OPTIMIZE_FAILED`, `LIBRARY_SAVED`, `LIBRARY_RECOVERY`.
- Unaffected 1080p H.264 videos (SPS uses up to four references) skip conversion. Library, search, tags, LiveShell switching, PS5 UI and previous fixes remain.
- Conversion uses open-source FFmpeg 7.0.1 and libx264 from PacBrew. GPL source and upstream notices remain available.
- CI validates a real MotionBGS HD source and archive restoration; final performance must be confirmed on PS5 hardware. After a cold power cycle, PS5 homebrew/payload execution still requires jailbreak/homebrew startup; the archive remains saved independently.
