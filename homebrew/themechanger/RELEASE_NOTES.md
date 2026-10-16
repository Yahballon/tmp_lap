# ThemeChanger v0.3.13 – Permanent Downloads & Playback Diagnostics

- **Persistent MP4 download fix:** new MotionBGS MP4s are first fully downloaded to `/data/ThemeChanger/wallpapers/`, then atomically copied into `/data/LiveShell/video/` for the LiveShell engine. They are preserved independently of LiveShell's working directory.
- **Fast restart:** `/data/ThemeChanger/wallpapers.index` records the downloaded filenames. At startup only these indexed filenames are checked and restored; no 9,823-path synchronous startup probe (which caused v0.3.11 startup instability).
- **Migrates existing downloads:** files from v0.3.12 in either LiveShell or the older ThemeChanger archive are slowly discovered, archived, and shown in Backgrounds without blocking the initial screen.
- **Independent copies:** disable hard-link backups, which might share modified inode content with active video. Use bounded 8 KB stack buffer, partial files, explicit flush/fsync and atomic rename.
- Adds `LIBRARY_RECOVERY`, `LIBRARY_INDEX_LOADED`, `WALLPAPER_ARCHIVED`, `LEGACY_WALLPAPER_IMPORTED` and `LEGACY_ARCHIVE_RESTORED` diagnostic events.
- Keeps stable v0.3.12 downloads, UI, the full MotionBGS search/tags catalog, and patched LiveShell home-screen behavior.
- **Video stutter remains a separate decoder limitation:** some 1080p MotionBGS H.264 files use eight reference frames where a FullHD hardware-decoder profile may support four; level-header changes make images appear but do not rebuild encoded frames. The experimental native FFmpeg transcoder remains disabled after the v0.3.10 crash. Obtain `/data/LiveShell/shellui.log` for exact watchdog timing and on-console verification.
