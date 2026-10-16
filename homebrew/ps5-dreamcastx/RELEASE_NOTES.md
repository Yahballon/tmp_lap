# PS5DreamcastX v0.1.7

## USB library regression hotfix — recover missing games

- Fixed a USB scan-root regression: if `PS5DreamcastX/games/`
  exists, v0.1.6 scans **only** that subfolder, hiding games
  in `PS5DreamcastX/` and its other subdirectories.
- Now recursively scans the entire `/mnt/usb0/PS5DreamcastX/`
  folder (or USB1–7). The existing `games/`, `Games/` and nested
  subfolders are included automatically, with no duplicate roots.
- Corrected uppercase Dreamcast ROM extensions: `.GDI`, `.CHD`,
  `.CDI` and `.CUE` are now accepted like lowercase extensions.
- Added a regression test simulating six games across root and nested
  folders, verifying all six are discoverable.
- Improved logging: separate *USB scan roots* from *detected media/titles*.
- Retained the nonblocking v0.1.6 cover decoder and v0.1.5 stable cases.
- No saves, BIOS, games, USB file or controller configurations are modified.

TheGamesDB HTTP 503 is an unrelated online cover-service failure. Existing
local cover files and ROM scanning do not require that API.

Native title ID: `PPSA99765`. Install ZIP: `PPSA99765.zip`.
