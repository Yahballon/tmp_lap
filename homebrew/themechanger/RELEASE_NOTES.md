# ThemeChanger v0.3.5 – PS5 Home Focus Fix

- Fixed the original PS5 background reappearing when users press the PS button to return to Home without fully closing ThemeChanger.
- Build a modified, auditable LiveShell ShellUI from GPL-3.0 upstream source. Daemon passes a ThemeChanger-only running-app marker, and ShellUI excludes PPSA99891 from its Game-layer video fade.
- Keep the game-pause and Game-layer fade behavior for other real games intact.
- Keep v0.3.4 protections: repeated MP4 selection, no redundant LiveShell Engine tile registration, MotionBGS previews/downloads, downloaded backgrounds, logging.
- Full PS5 reboot is necessary once after upgrading the bundled privileged LiveShell payload.
- Native CI compilation validates integration. Visible hardware behavior requires on-console testing.
