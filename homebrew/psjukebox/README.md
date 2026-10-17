# PSJukebox by YAHBALLON

Native PS5 offline music player with a five-cover album carousel, per-artist logo and colors, per-album changing background, track list and native FFmpeg/SDL2 music playback.

## Artist-customizable themes

The app scans \`/data/PSJukebox/Music\`, \`/download0/PSJukebox/Music\`, \`/mnt/usb0/PSJukebox/Music\`, \`/mnt/usb1/PSJukebox/Music\`, and \`/mnt/usb0/Music\`, \`/mnt/usb1/Music\`. The latter paths require USB filesystem access from the active homebrew runtime.

\`\`\`text
Music/
  Bob Marley/
    logo.png
    background.jpg                  # optional artist fallback
    theme.ini
    Legend/
      cover.png
      background.jpg
      01 - Is This Love.mp3
      02 - No Woman No Cry.flac
    Exodus/
      cover.jpg
      background.png
      01 - Natural Mystic.m4a
  Snoop Dogg/
    logo.png
    theme.ini
    Doggystyle/
      cover.jpg
      background.jpg
      01 - Gin and Juice.mp3
\`\`\`

The artist logo appears in the top-left header. Selecting an album changes the full-screen background (fallback: artist background; otherwise a colored gradient), and browsing artists switches logo/color/theme/album list. PNG/JPG/JPEG covers are supported; PNG wins when multiple are present. The fallback generated UI works even without artwork.

**theme.ini** (artist folder):
\`\`\`ini
accent=#FFC735
secondary=#173F37
text=#FFFFFF
\`\`\`
Colors are 6-digit RGB hex. The app uses \`accent\` for focus and badges, \`secondary\` for the gradient, and the default white/muted palette for readable fixed chrome.

## Controls

- Left/Right: browse album covers
- L1/R1 on album view: previous/next artist
- Cross: open selected album; on song: play; on player: pause/resume
- Up/Down in track list: select track
- L1/R1 during playback: previous/next song
- Left/Right during playback: seek 10 seconds
- Triangle: track details/list
- Square: rescan all music on the album view
- Options: settings
- Circle: back / exit confirmation

Supports audio decoders exposed by the PS5 FFmpeg build; supported container extensions include MP3, FLAC, M4A, AAC, WAV, OGG, OPUS and WMA. Codec support depends on linked FFmpeg; DRM files cannot be played.

## Installation and limits

Build workflow: \`.github/workflows/build-psjukebox.yml\`. CI produces an installable \`PPSA99947.zip\` test artifact. Extract the \`PPSA99947\` folder to \`/data/homebrew/\` and register with your supported homebrew loader. **A GitHub Actions build is not a PS5 hardware test.**

The first implementation relies on **directly readable** local/USB folders. Some sandboxed launchers cannot enumerate \`/data\` or \`/mnt/usb*\`. In those environments an authorized local filesystem helper will be required; it is not included in v0.1.0. Logs: \`/data/PSJukebox/psjukebox.log\`.

No copyrighted music/artist covers/backgrounds are shipped. Supply personal, licensed artwork/audio. The attached screenshot is a visual design reference, not redistributed third-party content.

License: GPL-3.0-or-later. Native player code adapted from GPL-3.0-or-later Audiobook Reader in this repository, renderer from PS5 homebrew UI. Version 0.1.0, Title ID PPSA99947.

## TESTBUILDS catalog release

A successful **main-branch** GitHub Actions build creates an immutable `psjukebox-v0.1.0` Release containing verified `PPSA99947.zip`. The existing schema-2 `version.json` catalog workflow discovers its release asset on Build PSJukebox completion. The TESTBUILD designation appears in the release title. The build has not yet been tested on physical PS5 hardware.

## v0.1.1: permission-denied USB music fix

When the log reports `UNAVAILABLE ... errno=1`, the app's sandbox does not have directory-enumeration permission. The included `jbxscan.elf` is a dedicated, **read-only** file helper that runs via your **already-running elfldr** service on `127.0.0.1:9021`. It serves the allowed music, `theme.ini`, logos and album artwork on loopback port `9188`.

The existing USB layout stays **PSJukebox/Music/<Artist>/<Album>** (or **Music/<Artist>/<Album>**) with `logo.png` and optional `theme.ini` in the artist folder; `cover.jpg/png`, `background.jpg/png`, and audio files in each album.

The helper is included inside the installable PSJukebox ZIP. No separate computer-side tool is needed. Songs are staged to `/data/PSJukebox/Cache/` on first playback (with progress and cancel), reused on future plays. USB originals are not modified. Artist-themed artwork is transferred as needed.

**Important:** You need a compatible jailbreak environment with `elfldr` already running on port 9021. Whether the helper can reach your particular USB mount is still firmware/loader dependent and requires PS5 hardware testing. A running app without that service cannot overcome filesystem permissions. App log: `/data/PSJukebox/psjukebox.log`.
