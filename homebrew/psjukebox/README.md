# PSJukebox — Native PS5 Music Player by YAHBALLON

A native, offline PS5 multi-artist music player with a cinematic five-album carousel, individual artist logos, album covers, fullscreen art backgrounds and per-artist color themes. No media or third-party artist imagery is bundled.

## Music layout

Place your own music and artwork on writable local storage or accessible USB:

```text
PSJukebox/
  Music/
    Bob Marley/
      logo.png
      background.jpg             # optional artist fallback
      theme.ini                  # optional
      Legend/
        cover.jpg
        background.jpg
        01 - One Love.mp3
        02 - Redemption Song.flac
      Exodus/
        cover.png
        background.png
        01 - Natural Mystic.m4a
    Snoop Dogg/
      logo.png
      theme.ini
      Doggystyle/
        cover.jpg
        background.jpg
        01 - Track Name.mp3
```

`cover.png` and `background.png` can instead be `.jpg` or `.jpeg`. PNG wins when both are present. An album missing `background.*` uses its artist's optional `background.*`; otherwise the app uses theme colors. Each artist has a separate `logo.png`. Folder names supply labels. Empty albums are hidden.

**theme.ini** (in artist root):
```ini
accent=#FFCC33
secondary=#174033
text=#FFFFFF
```

Music scan roots (searched independently):
- `/data/PSJukebox/Music` and `/download0/PSJukebox/Music`
- `/mnt/usb0..3/PSJukebox/Music`
- `/mnt/usb0..1/Music`

The app uses **direct read access** for these paths. Some firmware or sandbox setups cannot enumerate USB or `/data` directly. A privileged USB relay/helper is **not yet included**; diagnostics and console testing are required for those setups. Do not infer USB support solely from the scan paths.

## DualSense
- Left/Right: move between albums with cover/background switch
- L2/R2: change artist, switching logo, palette and background
- Cross: play selected album from track 1
- Triangle: open track selector; Up/Down and Cross choose track
- L1/R1 during playback: previous/next track
- Left/Right during playback: seek -/+ 10s
- Cross during playback: play/pause
- Circle: return to albums / cancel / exit confirmation
- Square: rescan library
- Options: controls/settings overview

## Native stack

Reuses GPL-3.0-or-later PS5-DevBridge `templates/simple-calc`, `ps5-homebrew-ui` OpenGL rendering, FFmpeg and SDL2. Source lives in `homebrew/psjukebox`; the GitHub Actions workflow is `.github/workflows/build-psjukebox.yml`. The workflow compiles the native executable and uploads a ZIP test artifact if successful. No release is automatically published.

Version 0.1.0 | Candidate Title ID PPSA99947 (not an official global reservation).

### Initial limitations
- Artwork loads on first use; large images could briefly stall the rendering thread. This is an initial implementation, not yet an asynchronous artwork pipeline.
- Audio decoders depend on the FFmpeg codecs actually present in the packaged PS5 library; the recognized extensions do not guarantee playback for every encoding or DRM file.
- Direct USB mount access is firmware/runtime dependent. Current app has no USB transfer helper.
- Visual and hardware integration remain unverified until an actual successful PS5 build and console test.
- Playback/selection state does not persist between launches in v0.1.0.

Source license GPL-3.0-or-later. User-provided artwork and music remain the property of their respective rights holders.
