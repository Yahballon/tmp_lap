# ZQuest Classic PS5 v0.1.0 — Quest Browser Prototype

**Experimental PS5 TESTBUILD · by YAHBALLON · PPSA99127**

**NOT PLAYABLE:** This release contains a native PlayStation 5 quest-browser UI
only. It does **not** include the upstream `zplayer` game engine, quest
execution, Zelda gameplay, music playback or save-data compatibility.

## What you can test
- Boot and display scaling of a native 1920×1080 controller-first menu.
- D-pad/left-stick quest selection, Cross details, Square rescan, Options status
  and Circle/back/exit confirmation.
- Bounded incremental discovery of `.qst` files on writable storage and USB.
- Detection of non-existent, empty or inaccessible game directories.
- Non-destructive log collection in `/data/ZQuestClassic/logs/port.log`.

## Installation
Use the TESTBUILDS app to install `PPSA99127.zip` from the GitHub Release, or
unzip `PPSA99127/` beneath `/data/homebrew/` via a compatible loader.
Suggested quest locations:
`/data/ZQuestClassic/quests/`, `/mnt/usb0/ZQuestClassic/quests/`.

## Verification
Source host scanner tests, native PS5 cross-compile and executable/metadata/
icon/ZIP checks are GitHub Actions gates. **PS5 hardware execution has not yet
been verified**. Upstream game engine integration is a separate milestone.

## Provenance
Native UI code: YAHBALLON, GPL-3.0-or-later.
Upstream engine project: ZQuestClassic/ZQuestClassic (GPLv3), *not linked into
this release*. No copyrighted Nintendo ROMs or commercial quest assets shipped.
