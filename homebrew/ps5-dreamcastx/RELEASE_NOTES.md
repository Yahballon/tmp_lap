# PS5DreamcastX v0.1.1

## Changed

- Native app installation and content folder is now `/data/homebrew/PS5DreamcastX/`.
- Game backups can be placed in `/data/homebrew/PS5DreamcastX/games/`.
- Automatically discover `/mnt/usb0/PS5DreamcastX/` through `/mnt/usb7/PS5DreamcastX/`, selecting the `games/` subfolder when present.
- USB access is on by default for **new** installs; enable `USB drives` and restart if an existing `frontend.cfg` has `usb = 0`. Requires elfldr on port 9021.
- Keep other Flycast USB folder names, and save/BIOS/cover support.
- The **PS5 title ID and the TESTBUILDS ZIP filename remain PPSA99765** as required by existing metadata and catalog discovery.
- Self-update now expects a package rooted at `PS5DreamcastX/`.

**Migration warning:** v0.1.0 cannot rename its installed directory through self-update. Stop the app, move the previous PPSA99765 install folder to PS5DreamcastX preserving user data, and install this new ZIP over it.

Based on native PSFlycast (GPL-2.0-or-later), with original notices preserved.
