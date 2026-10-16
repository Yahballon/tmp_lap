# ThemeChanger v0.3.3 – Playback Reliability & LiveShell v1.1

- Bundles verified upstream LiveShell v1.1 ELF, replacing v1.0 in the installer.
- Fixes repeated Cross presses after long blocking or empty scePadRead controller reads.
- Increases video-selection API timeout to 20 seconds for ShellUI video release and remount operations.
- Auto-enables Main Video on global theme selections (LiveShell v1.1).
- Verifies the selected filename and LiveShell enabled state; distinguishes API acceptance from actual ShellUI playback.
- Logs `SHELLUI_MONITOR`, `PLAYBACK_STATE`, API selection details, codec hints and legacy engine detection to `/data/ThemeChanger/themechanger.log`.
- Adds a mock HTTP LiveShell integration test to GitHub Actions, retaining MP4, JPEG and PS5 package verification.
- Important: restart the PS5 and reapply jailbreak once so the newly bundled v1.1 engine replaces any resident v1.0 daemon.
- Console testing is needed to verify real ShellUI injection/rendering; no background change is guaranteed by HTTP acknowledgement alone.
