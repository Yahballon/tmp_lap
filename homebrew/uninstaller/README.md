# Uninstaller by YAHBALLON

Native PS5 Homebrew v0.1.3 | Title ID PPSA99986

Native neon blue-and-red dashboard inspired by the provided reference. This app has four tabs (All, Games, Homebrew, USB PKGs), a live file inventory, detail pane, storage capacity when available, full DualSense menu controls, and confirmation dialog.

## Verified scope of source functionality

- Detects accessible title directories in /user/app, /system_ex/app, /data/homebrew, /data/etaHEN/games, /user/data/etaHEN/games, /mnt/usb0..7 and /mnt/ext0..3 (homebrew, etaHEN/games and external user/app).
- Supports nested game-folder layouts, -app/-app0 suffixes and PPSA/CUSA/LAPY/FAKE IDs; also recognizes custom folder names with eboot.bin, sce_sys/param.json or sce_sys/param.sfo.
- Finds .pkg archive files up to two folder levels down on readable external drives and distinguishes them from installed titles.
- Removes only selected homebrew title folders or USB .pkg files, with explicit confirmation and strict root/path and symlink validation.
- Filesystem capacity is displayed only when statvfs returns usable values; folder sizes are not invented.
- Keeps game saves untouched and logs success/error messages under /data/Uninstaller/uninstaller.log and a signal report under /data/Uninstaller/crash.log.
- **Important: installed retail/PKG titles cannot yet be uninstalled through a native system API in this version.** Their Uninstall action is blocked pending verified privileged AppInstUtil integration. This app never removes system app directories directly.
- Deleting a USB .pkg archive does NOT uninstall an already installed PS5 title.

## Controller

D-pad up/down: selection; D-pad left/right or L1/R1: categories; X or Square: confirm selected item; Circle: cancel/exit; Triangle: refresh.

## Build & install

CI: [Build PS5 Uninstaller](../../.github/workflows/build-uninstaller.yml), based on the [PS5-DevBridge](https://github.com/Yahballon/PS5-DevBridge) simple-calc native app scaffold.

A successful CI run supplies an artifact PPSA99986.zip. Extract its title folder to /data/homebrew/ on a supported jailbroken PS5 and register it with a compatible homebrew loader.

The supplied screenshot is a visual reference, not a real PS5 capture. Native build and PS5 hardware validation have not yet been confirmed.

## Files

- native/main.cpp: OpenGL UI and controller handling
- native/inventory.hpp: filesystem catalog and deletion guard
- tests/inventory_test.cpp: path safety regression tests
- assets/make_icon.py: original neon red icon generator for sce_sys/icon0.png
- app.json: title metadata

GPL-3.0-or-later, matching PS5-DevBridge homebrew UI. Developer: YAHBALLON.

**Critical:** The old v0.1.0 archive used title ID PPSA99873, colliding with Payload LauncherX. Do not install that superseded archive. Always use v0.1.3 (PPSA99986) or later.

## Crash diagnostics in v0.1.2

- Writes early startup milestones to `/data/Uninstaller/uninstaller.log` with immediate flush/close, before PS5 graphics initialization. Records graphics, font, input, first frame and per-storage scan roots.
- Saves best-effort SIGSEGV/SIGBUS/SIGILL/SIGABRT registers and backtrace to `/data/Uninstaller/crash.log` through the PS5-DevBridge crash reporter. The OS may prevent report creation for failures before this handler installs.
- Delays storage scanning until after the first frame so the last marker shows which initialization stage failed.
- Disables speculative `statvfs` capacity probing on PS5, because its runtime implementation has not been verified; the capacity indicator shows unavailable instead of risking the app.
- Logs remain across restarts, so share both files after any crash. This is a diagnostic hardening release; an on-console hardware test is still required to confirm the crash cause and outcome.

## v0.1.3 – discovery fix after 0 entries on PS5

The previous v0.1.2 scanner only accepted exact PPSA/CUSA top-level folders and did not show whether a root was readable, missing or permission-denied. The current bounded scanner searches the additional documented ShadowMount/etaHEN roots, title suffixes and custom-named homebrew directories. It logs SCAN_ROOT_BEGIN, SCAN_ROOT_ERROR (with errno), SCAN_ROOT_DONE (visited/found), SCAN_FOUND and PKG_ROOT_ERROR/DONE into /data/Uninstaller/uninstaller.log.

The OS's system SQLite app.db is only probed for path existence; it is NOT parsed or modified. Consequently, native PS5/PS4 titles that cannot be traversed from the app sandbox may still be invisible. A future privileged, read-only inventory helper would be needed in that situation.

Game dump folders and system-installed titles are shown as read-only, never recursively deleted. Homebrew deletion continues to require an exact top-level title folder under an allowlisted root with an eligible ID; the app itself is protected. USB package deletion still requires explicit confirmation.
