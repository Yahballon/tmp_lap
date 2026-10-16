# ThemeChanger v0.3.6 – LiveShell Engine Handover

- Fixes repeated stock background when pressing PS button with ThemeChanger left running by addressing the confirmed root cause: an old LiveShell daemon was still active despite the packaged patch.
- Changes the integrated source-built LiveShell /api/state response to report a distinct engine version.
- Automatically upgrades older/incompatible LiveShell instances using LiveShell's own supported handover when elfldr port 9021 is available.
- Prevents misleading success reports when the app has contacted an old daemon and ShellUI is stopped.
- Displays handover progress and records diagnostic log entries.
- Keeps previous focus-layer fix, game pause support for real games, no redundant Media tile, and MotionBGS functionality.
- Full PS5 reboot may still be necessary if an existing payload does not support handover or if elfldr is unavailable.
- Native compilation and API tests do not guarantee visible hardware playback; test on console.
