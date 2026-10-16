# ThemeChanger v0.3.8 – Downloads-Only Backgrounds Library

- Removed all 12 bundled default wallpaper choices from the Backgrounds tab.
- Removed the associated 12 JPG/PNG theme previews and 12 sample MP4 clips from the installation ZIP, reducing package size.
- Backgrounds now lists **only downloaded MotionBGS MP4s**, with the newest downloads displayed first.
- Added an immediate download-completed event from the catalog worker to the UI: a newly downloaded wallpaper appears in Backgrounds without restarting or waiting for periodic re-scan.
- Downloaded wallpapers remain stored under `/data/LiveShell/video/MotionBGS-*.mp4` and can be selected/reapplied directly from Backgrounds.
- Added friendly empty-library instructions pointing to Online Wallpapers.
- Existing online MotionBGS gallery (~9K+ titles), search, tags, thumbnails, HD video downloads, integrated LiveShell 0.3.6 engine handover and ShellUI fixes remain.
- Old `ThemeChanger-*.mp4` sample clips from earlier versions are not deleted automatically; they are simply no longer shown in the ThemeChanger UI.
- The PlayStation 5 hardware UI behavior still requires final on-console verification.
