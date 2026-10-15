# PSTUBE v0.6.0 — Seek, Fullscreen and Jailbreak News

- **Default discovery search:** PS5 JAILBREAK NEWS.
- **Rewind / forward:** D-Pad Left / Right moves by 10 seconds, L1 / R1 by 30 seconds while playing.
- **True full-screen video:** Press Options while playing to toggle edge-to-edge video with original aspect ratio. Transient controls automatically hide when playing.
- **Playback seek implementation:** FFmpeg does the seek asynchronously; it flushes video/audio codec and SDL audio buffers before resuming timestamp-synchronized playback.
- Show progress and duration plus controller hints in both normal and full-screen modes.
- Preserve existing native player, online YouTube search, auto-pagination, thumbnails, downloads and saved settings.

Limitations: seeking is available only on seekable MP4 files or HTTP servers supporting byte-range requests. The PS5 hardware test is still necessary; successful CI does not prove all YouTube streams allow seeking.

Install the new PPSA99762.zip in /data/homebrew/PPSA99762. No PC bridge or additional user installation required.
