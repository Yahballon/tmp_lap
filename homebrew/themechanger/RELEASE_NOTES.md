# ThemeChanger v0.3.12 — Emergency Boot Recovery

**Critical crash regression fix** for PS5 `system_illegal_function_call` when starting ThemeChanger after v0.3.10/v0.3.11.

- Restores the complete **last console-confirmed working v0.3.9** boot, download-catalog and DualSense gallery code.
- Disables all unverified boot-time file recovery code which invoked PS5-incompatible/unverified filesystem operations, and removes the experimental FFmpeg/x264 converter from the compiled PS5 executable.
- Keeps the integrated LiveShell controller engine, 9K+ MotionBGS wallpaper index, search, tags, downloaded-only Backgrounds library and H.264 metadata compatibility fix as in v0.3.9.
- No automatic deletion of downloaded wallpapers. Existing /data/LiveShell/video/ files and backups under /data/ThemeChanger/wallpapers remain untouched. This boot recovery deliberately **does not** attempt the unstable persistent restore introduced in v0.3.10.
- Adds a CI guard to prevent unsupported startup recovery and experimental transcoding accidentally being linked into the emergency release.
- PS5 console boot confirmation is still required; successful GitHub compilation cannot prove the device has no illegal function calls.

For users currently unable to start, update to this ZIP, **fully close the old app**, and launch ThemeChanger again. If the app remains unable to launch, collect the earliest crash/loader message and the current `/data/ThemeChanger/themechanger.log` and specify if a new BOOT line was written.

Some previously jerky H.264 streams can still stutter because experimental native re-encoding remains disabled for stability.
