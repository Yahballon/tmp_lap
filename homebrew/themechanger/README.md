# ThemeChanger - PS5 Homebrew

Native theme chooser for PlayStation 5 homebrew. Reproduces the provided dark blue screenshot as a PS5 native 1920x1080 menu, built with [ps5-homebrew-ui](https://github.com/blackbearreloaded/ps5-homebrew-ui) and the [PS5-DevBridge](https://github.com/Yahballon/PS5-DevBridge) SDK framework.

## LiveShell integration
Uses the documented [LiveShell by ZiZc3](https://github.com/ZiZc3/LiveShell) GPL-3.0 REST endpoints on localhost:9050. It installs selected MP4 clips to `/data/LiveShell/video/ThemeChanger-*.mp4`, reads `/api/state`, and calls `/api/select?i=N`. When LiveShell is not running the app shows a clear message, instead of claiming PS5 firmware was modified. **Since v0.2.0 the release ZIP bundles LiveShell v1.0 and automatically launches it through existing elfldr (127.0.0.1:9021). No manual LiveShell loading is required once elfldr is running.** The LiveShell payload itself requires its supported OnionHEN/ShellUI environment and is **not** bundled.

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
