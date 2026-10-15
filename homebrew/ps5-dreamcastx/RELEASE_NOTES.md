# PS5DreamcastX v0.1.2

## Fixes

- The internal library and user content now use **`/data/PS5DreamcastX/`**:
  `games/`, `bios/`, `covers/`, `data/` (saves/VMUs), logs, cheats,
  configuration, shaders and network settings. App binaries remain under
  `/data/homebrew/PS5DreamcastX/` as a separate install.
- The app attempts filesystem elevation (elfldr port 9021) even when USB
  browsing has been disabled, so the external data folder is accessible.
- Corrected the cover-flow renderer: use Flycast's working `drawCover()`
  and aspect-correct `imageFill()` so portrait artwork fills the game case,
  rather than appearing tiny in its top-left corner.
- Preserve USB auto-discovery under `/mnt/usb0..7/PS5DreamcastX/`.
- Ensure video-output metadata and in-app updater still reference the
  **installed** title, never the writable data directory.
- Correct the branded build-version parser.
- The unchanged PS5 title ID and TESTBUILDS ZIP filename are `PPSA99765`.

**Data migration:** Copy existing games, BIOS, covers and save files from
the previous app directory to `/data/PS5DreamcastX/`. No user data is
removed or automatically moved. On systems without /data access the app
falls back to the mounted application directory.

Based on PSFlycast. Original upstream GPL notices remain intact.
