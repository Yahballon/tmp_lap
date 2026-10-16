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
