# Uninstaller by YAHBALLON

Native PS5 Homebrew v0.1.10 | Title ID PPSA99986

Native neon blue-and-red dashboard inspired by the provided reference. This app has four tabs (All, Games, Homebrew, USB PKGs), a live file inventory, expanded selected-title detail pane, full DualSense menu controls, and confirmation dialog.

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

**Critical:** The old v0.1.0 archive used title ID PPSA99873, colliding with Payload LauncherX. Do not install that superseded archive. Always use v0.1.10 (PPSA99986) or later.

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

## v0.1.4 – LauncherX-style elfldr inventory bridge

The PS5 console logs showed EPERM (errno 1) at /data/homebrew, /mnt/usb0 and the system app database. The sandboxed graphical app could not enumerate these roots. The separately proven Payload LauncherX uses a bundled, read-only USB scanner ELF sent to a **pre-existing elfldr at localhost:9021**. Uninstaller now follows this architecture.

- `helper/unscan.c` is a standalone **read-only** ELF: bounded filesystem enumeration and metadata reporting, no delete/install/write/mount commands. Outputs binary-length-prefixed inventory records over elfldr's stdout.
- `native/helper_bridge.hpp` sends the ELF bytes to localhost:9021, validates the returned protocol, merges discovered titles into the UI and logs per-root access results.
- Helper-derived records are flagged **remote=true**. Every such record is **non-removable**, including USB PKG archives. This is an inventory/discovery release only. A separate, carefully reviewed privileged uninstall protocol would be required later.
- Root status is logged in `/data/Uninstaller/uninstaller.log` as `ELFLDR_ROOT`, plus `ELFLDR_FOUND`, `ELFLDR_ERROR` and `ELFLDR_RESULT`.
- The UI still works without elfldr; it displays the reason if the local service is missing. An active elfldr does not itself guarantee access to the installed game database.
- Requires a **compatible trusted elfldr already running** on 127.0.0.1:9021, like LauncherX; do not connect it to untrusted remote payload services.

## v0.1.5 – real console storage and game/app icons

- Native **Console Storage** uses filesystem capacity measured by the trusted ELF loader's read-only `statfs` on `/user` and mounted external volumes, NOT the unprivileged title's unreliable `statvfs` implementation. Values that are missing or implausible are displayed as Unavailable. L2/R2 cycles measured volumes.
- Restores the blue storage usage ring, actual used/total/free figures and storage-selection label.
- Display real `sce_sys/icon0.png` logos in the left game/homebrew list and the selected item's detail panel. Fallbacks try `/user/appmeta/<title_id>/icon0.png` and `/system_data/priv/appmeta/<title_id>/icon0.png` including their external variations. Unsupported icons use G/H/P placeholders.
- `unscan.elf` now runs a temporary localhost read-only icon server on port 9175; the title requests only an enumerated item's numeric ordinal, never arbitrary file paths. Logos are fetched lazily in a worker, decoded in the OpenGL render thread and held in a small in-memory texture cache. No icons are downloaded from the Internet or saved on disk.
- Logs `ELFLDR_STORAGE` and `ELFLDR_STORAGE_ERROR` under `/data/Uninstaller/uninstaller.log`.
- This update intentionally **does not enable uninstall of system-registered titles**. It also does not remotely delete USB files; visible remote entries remain read-only. Further safe privileged removal integration requires separate testing/confirmation.

## v0.1.6: improved UI, reliable storage filtering, experimental removals
- Removes the permanent "Found X entries" floating notification; successful refresh remains in the category count and logs. Errors are shown as a subtle status line near the footer.
- Rejects misleading mount-point placeholder capacity such as /mnt/ext0 showing only 2 MB. Only drives with believable capacity (internal >=128GiB, extended >=8GiB, removable >=128MiB) are displayed. If no trustworthy drive is accessible, panel reports Unavailable rather than inventing figures.
- Adds a separately compiled **unremove.elf** opt-in action runner, never invoked during scans. Requires a strict 0700 private request directory /data/Uninstaller/actions, random 128-bit nonce, explicit title/path validation, 2-step user confirmation, a verified response and correct owner permissions.
- Registered installed titles at exact /user/app/<titleId> are sent to `sceAppInstUtilAppUnInstall`, not recursively deleted. The helper uses installer authid only around this call and restores the previous authid. A zero API return is *request accepted*, not proof of completion; the user must refresh and confirm disappearance.
- USB PKG archives under /mnt/usb0..7 are eligible for explicit remove after strict lstat validation. Deleting a USB archive does not uninstall the registered copy.
- Custom game dump mounts or homebrew folders accessed only through the privileged helper remain read-only because recursive folder deletion may destroy unrelated assets. No game saves, app databases, or system folders are manually modified.
- Experimental PS5 system uninstall: source compiles, but firmware-specific execution results are NOT guaranteed. First test using a disposable homebrew app rather than a purchased game. Log details at /data/Uninstaller/uninstaller.log.

## v0.1.7 — nonblocking scan progress

- The first scan and Triangle refresh run on a worker thread. The main renderer continues drawing a blue animated progress bar with truthful, phase-based status. Percentage is intentionally not fabricated: elfldr does not expose a verified total count during transmission.
- Controller actions are temporarily locked while the inventory is being rebuilt, preventing stale title selection and accidental deletion. Completed items and volumes are transferred back to the renderer thread before use.
- The existing explicit removal confirmation and AppInstUtil/USB-PKG backend from v0.1.6 remain present. Hardware confirmation of native uninstall success is still required; do not assume accepted system requests have completed.
- Existing protections remain: no raw unlink of system title folders, no save-data removal, and the application's own title ID cannot be removed.
- Diagnostic logs: `/data/Uninstaller/uninstaller.log`.

## v0.1.10 — working deletion bridge hardware test

The v0.1.7 PS5 console log confirms `REMOVE_PRECHECK Action folder is not private; aborting` on every installed-title and USB-PKG removal attempt. The graphical app cannot safely own `/data/Uninstaller/actions` in its sandbox, even with elfldr.

v0.1.10 no longer uses that directory. Instead it sends a single-use `unremove.elf` to the existing localhost:9021 elfldr; the ELF creates a 128-bit random capability, announces it on its existing private loader stream, listens temporarily on **127.0.0.1:9176**, and accepts just one authenticated, explicitly confirmed action. It returns the native return code immediately to the UI and logs `REMOVE_HELPER` and `REMOVE_RESULT`.

- Installed native PS5/PS4 registered titles: `sceAppInstUtilAppUnInstall` request for a validated title actually present under `/user/app`; zero means request accepted, **not proof that asynchronous removal completed**. Saves are not targeted.
- USB PKG archives: carefully validated standalone `.pkg` file removal. It does not uninstall the installed copy.
- USB homebrew folders: only immediate child directories at `/mnt/usbN/homebrew/<folder>`, with no symlinks/special files and a bounded two-phase traversal. Does not affect separately installed applications.
- Unsupported paths, protected/system title roots, and the Uninstaller's own title remain non-removable.
- The user must explicitly select the entry and confirm. No removal occurs during a scan, refresh, thumbnail read, or normal startup.
- **Hardware testing required:** begin with an expendable USB homebrew folder or an unimportant PKG, then test an installed throwaway homebrew title on your own PS5 firmware. See `/data/Uninstaller/uninstaller.log`.

## v0.1.10 installed-title diagnostics

Game uninstall now calls the SDK-linked one-argument Sony API, matching the Playden title-remover payload. Logs record `REMOVE_STAGE` for title validation, AppInstUtil initialization, installer authid switch and uninstall IPC. If the helper ends unexpectedly, the last stage identifies the likely failure point. Test only an expendable installed game first; v0.1.10 is compiled but game uninstall on firmware 5.10 is not yet hardware-confirmed.

## v0.1.10 — Clean two-column UI

- Removed the full Console Storage box and the now-unused L2/R2 drive switcher.
- Larger selected-title logo and information on the right, with a wide uninstall button.
- Existing game uninstall, USB homebrew and USB PKG deletion paths are **unchanged**. Scan progress, title logos and logs remain available.

## PPSA21564 ShadowMountPlus source deletion (v0.1.10)

ShadowMountPlus can re-add the same game after uninstall if its original folder or backing image remains under a watched scan root. Uninstaller v0.1.10 has an **optional, separately confirmed ShadowMount source-delete path** for the requested `PPSA21564` title. Select that game's entry and press **X**, review the source path returned by ShadowMountPlus, then press **X** again to authorize permanent deletion. **Square** still performs the regular game/app uninstall. The source and the installed title are different things: deleting the source removes the actual game dump/image, while regular uninstall only unregisters its tile.

This uses the official ShadowMountPlus 1.7 API on `127.0.0.1:10101` (`/api/v1/version`, `/api/v1/games/info`, `/api/v1/games/delete`, `/api/v1/games/storage/status`). The app checks the `delete_game_source` capability, game ID and ability to manage the actual source, then asks for confirmation. The response is asynchronous; progress is displayed and errors appear in `/data/Uninstaller/uninstaller.log`. No direct unlink, recursive directory deletion or destructive fallback is attempted by this new feature. If ShadowMountPlus 1.7 is unavailable, the function stays unavailable without touching data.
