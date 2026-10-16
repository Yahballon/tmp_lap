# ThemeChanger v0.3.11 – Download Crash Hotfix

- Fixes a regression where the PS5 exited unexpectedly during/just after MotionBGS downloads introduced in v0.3.10.
- Temporarily disables the **experimental built-in FFmpeg/x264 re-encoding** on PS5. The previous v0.3.9 Level 5.0 H.264 metadata compatibility patch remains for existing wallpaper playback. Some problem videos may still stutter; smooth transcoding will be revisited in a separate, instrumented update.
- Moves the 65 KB H.264 SPS inspection buffer to the heap (the buffer formerly risked a stack overflow in the smaller PS5 network thread).
- Explicitly allocates 2 MB native worker-thread stack for safer HTTPS and media tasks.
- Adds DOWNLOAD tracing: VIDEO_TRANSFER_DONE, VIDEO_TRANSFER_FAILED, VIDEO_NATIVE_CONVERSION_DISABLED and existing DOWNLOAD_COMPLETE to distinguish TLS/download faults from media handling.
- Fixes empty libraries after reboot by no longer depending on PS5 `opendir/readdir`. Saves a durable filename index at `/data/ThemeChanger/wallpapers.index`, verifies known entries using `stat`, and automatically restores available backed-up MP4s from `/data/ThemeChanger/wallpapers/`.
- Backgrounds updates immediately for newly downloaded videos. The 9K+ MotionBGS catalog, search, tags, LiveShell engine and wallpapers remain.
- Existing files are **not deleted**. Downloads interrupted by v0.3.10 can be restarted.
- CI verifies native compilation, persistent MP4 recovery, HTTPS download and preservation of safe-mode behavior. Console confirmation required.
