# ThemeChanger - PS5 Homebrew

Native theme chooser for PlayStation 5 homebrew. Reproduces the provided dark blue screenshot as a PS5 native 1920x1080 menu, built with [ps5-homebrew-ui](https://github.com/blackbearreloaded/ps5-homebrew-ui) and the [PS5-DevBridge](https://github.com/Yahballon/PS5-DevBridge) SDK framework.

## LiveShell integration
Uses the documented [LiveShell by ZiZc3](https://github.com/ZiZc3/LiveShell) GPL-3.0 REST endpoints on localhost:9050. It installs selected MP4 clips to `/data/LiveShell/video/ThemeChanger-*.mp4`, reads `/api/state`, and calls `/api/select?i=N`. When LiveShell is not running the app shows a clear message, instead of claiming PS5 firmware was modified. **Since v0.2.0 the release ZIP bundles LiveShell v1.0 and automatically launches it through existing elfldr (127.0.0.1:9021). No manual LiveShell loading is required once elfldr is running.** The bundled LiveShell engine still requires compatible ShellUI privileges and a running jailbreak ELF loader; no separate LiveShell installation is necessary.

## Controls
- D-pad / left stick: browse
- Cross: apply wallpaper (LiveShell must be active)
- Circle: return to backgrounds or exit
- L1: backgrounds; R1: change tabs
- Settings: toggle wallpaper, set dim, enable game pause or system settings suppression

## Files
- `native/main.cpp` native UI
- `native/liveshell_client.*` localhost LiveShell adapter
- `assets/generate.py` builds original previews and sample MP4s
- `app.json` metadata for TESTBUILD manifest

## Build
Push to GitHub `main` and use **Build PS5 ThemeChanger** workflow. Once completed, download the `PPSA99891.zip` release and extract to `/data/homebrew/`. The TESTBUILD manifest automatically discovers a release with tag `themechanger-v0.2.0`.

## Limitations
The preview icons are illustrative, not extracted from the installed PS5 dashboard. LiveShell v1.0 supports **video** backgrounds: still images are packaged as short looping H.264 clips. Actual wallpaper injection is performed by the separate LiveShell payload, not by ThemeChanger alone.

## License
GPL-3.0-or-later for this frontend, GPL-3.0 compatibility with LiveShell. See upstream licenses and credit.

## Integrated engine details

See [LIVESHELL_INTEGRATION.md](LIVESHELL_INTEGRATION.md) for source, license, checksum, installation, requirements, and troubleshooting.

## Online Wallpapers – MotionBGS (v0.3.0)

Browse publicly listed **1920x1080 MP4** wallpapers from [MotionBGS](https://motionbgs.com/) inside the native PS5 ThemeChanger interface. The GitHub Actions build discovers public item pages and official HD download links and packages **only metadata**, not third-party artwork or videos. Users download each video directly from the website to their PS5.

- **R1** cycle into Online Wallpapers; **L2/R2** filter categories (All, Featured, Anime, Games, Nature, Cars, Space, Technology, Fantasy)
- **Triangle** next page; **D-pad** navigate; **X** download/install (or apply when previously installed)
- TLS verified against CA roots, official-domain restrictions, 160MB size cap, progress meter, atomic `.part` downloads, MP4 signature validation
- Stored files: `/data/LiveShell/video/MotionBGS-<slug>.mp4`
- Thumbnails are fetched on-device to `/data/ThemeChanger/cache/`; if they cannot be decoded, show a placeholder
- After download, the LiveShell API automatically receives the video selection
- Built-in original themes and bundled LiveShell v1.0 remain unchanged

**Notes:** The online catalog is a snapshot refreshed during app builds, not an always-live search index. MotionBGS availability, CDN endpoints and copyright permissions may change; users should respect the rights of creators and site terms. LiveShell currently supports 1080p MP4, so 4K links are excluded.

## Controller input fix (v0.3.1)
Both the D-pad and the left analog stick now use edge-triggered input and controlled key repeats instead of treating every analog frame as a fresh press. This prevents unexpected rapid selection jumps when browsing large MotionBGS lists.

## v0.3.2: Gallery, preview troubleshooting and controller access

- Press **R1** once from Backgrounds to open Online Wallpapers, **L1** to return to Backgrounds. The sidebar remains accessible with Left from the first column, then Up/Down and X.
- Downloaded MotionBGS MP4s appear after the built-in themes in **Backgrounds**, including additional 12-item pages. Select with **X** to reapply through LiveShell.
- App diagnostics are written to `/data/ThemeChanger/themechanger.log` (rotates at 2MB to `themechanger.old.log`). Events include catalog, preview URL, download, image-decode, selection and LiveShell states. Send this log when thumbnails remain blank.
- Online previews use still artwork from the source webpage where offered. The build-time catalog adds a fallback for normal JPEG/PNG poster images when metadata is absent; no copyrighted preview files are bundled.

## v0.3.3 – Reliable wallpaper selection and ShellUI diagnostics

- PS5 controller: Uses newest scePadRead sample, preserves held state when the API returns zero samples, avoiding repeated Cross / theme activation after long LiveShell API calls.
- Official bundled LiveShell **v1.1** ELF (SHA-256 pinned). Important: if a previous v1.0 daemon is still running, **restart the PS5, re-enable your jailbreak and start ThemeChanger again** to use the new engine. No forced ShellUI restart is attempted.
- The application verifies LiveShell's `current`, `enabled`, `mainvideo`, and ShellUI `status` fields before reporting selection confirmed. Successful API response is **not** proof that the OS is visually drawing the video.
- Explicit global theme selection enables LiveShell's `mainvideo` when disabled (v1.1 API).
- Select timeout raised from 2 to 20 seconds: LiveShell can wait up to 8 seconds for the old ShellUI video to be released.
- Detailed diagnostics in `/data/ThemeChanger/themechanger.log`: `SELECT_REQUEST`, `SELECT_RESPONSE`, `PLAYBACK_STATE`, `SHELLUI_MONITOR`, `VIDEO_INFO`, `ENGINE_LEGACY`.
- If selecting works according to the REST API but the actual home screen stays unchanged, provide **both** the ThemeChanger log and `/data/LiveShell/liveshell.log`; include `/data/LiveShell/shellui.log` if present. Physical PS5 validation is required.

## v0.3.4 – reliable wallpaper switching and no redundant Media tile

LiveShell's normal `gamepause=1` logic treated ThemeChanger `PPSA99891` as a running game. During a wallpaper change it attempted to stop the previously playing video and sometimes timed out with `The home screen did not release the old video`. Our **source-built LiveShell integration** exempts only ThemeChanger from game detection while preserving game pause for actual games. Fast-switch should then work normally with a shared `/data/LiveShell/video/` mount.

Upstream LiveShell v1.1 by ZiZc3 is cloned during CI and patched in `liveshell_build/patch_liveshell.py`. Its normal `install_tile()` startup call is disabled: ThemeChanger alone manages the engine, so no *new* `LiveShell Engine` Media tile is registered. **Previously installed** `LVSH00001` may be removed once using the PS5 interface; we do not silently delete a registered PS5 app.

After installing v0.3.4 **fully reboot the PS5, enable the jailbreak and start ThemeChanger** to replace the older running daemon. Logs: `/data/ThemeChanger/themechanger.log` and `/data/LiveShell/liveshell.log`.
