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

## v0.1.2 — Multi-artist navigation and controller footer

- Artist folders appear even if there are no readable tracks or album folders yet (look for `ARTIST=... ALBUMS=... TRACKS=...` in `psjukebox.log`). Empty albums also remain in the list, so you can diagnose missing songs.
- Tracks directly under `Music/Artist/` appear as a virtual **Singles** album. The standard `Music/Artist/Album/track.mp3` layout remains recommended.
- Combine directly scanned and helper-scanned artists without duplicate artist cards.
- Both DualSense sticks and D-pad navigate covers. D-pad L1/R1 cycles artists. The selected album always stays centered while neighboring covers rotate around it and wrap at the ends.
- Footer uses geometrically drawn, colored PlayStation-style input symbols; removed the `Multi Album Player` subtitle from the header.
- Rebuilt release `psjukebox-v0.1.2` is auto-discovered by TESTBUILDS `version.json` after the main workflow passes.

## v0.1.3 — Cinematic interface and persistent Favorites

Screenshot-inspired full-window album UI with five working tabs: All Albums, Artists, Tracks, Playlists (the current album's playable queue), and Favorites. The USB tab is removed. Artist-defined `logo.png`, `background.jpg/png` and album covers remain the only art sources; the install ZIP does not bundle commercial cover art.

The selected album stays centered in the 5-jacket carousel. When a song plays, a mini-player with live song/album details, elapsed/total time and progress appears below it. The PS5 clock is shown at top-right when available.

**Controller:** L1/R1 switches artists in All Albums. Left stick, right stick, or D-pad Left/Right changes albums. Press **Up** in All Albums to focus the five header tabs, then Left/Right and X to activate. Alternatively L2/R2 cycles tabs while browsing. X starts the selected album; while playing it toggles pause. Triangle marks/unmarks the selected or playing song. In Favorites, Up/Down selects a saved song, X plays it, Triangle removes it. Square opens Settings (including Rescan); Circle returns.

Favorites are stored under **`/data/PSJukebox/favorites.json`** and persist across restarts. Entries use root-independent `Artist/Album/Song` identities, so moving a USB stick between mount indexes does not discard favorites. Changes are saved with a temporary file + atomic rename; a save failure is reported on screen and in `psjukebox.log`, not silently claimed successful.

**Notes:** The Playlists tab currently presents the current album's play queue. Named custom playlist creation is not implemented. This is an initial screen-layout implementation based on the supplied mockup, not a pixel-perfect screenshot clone. Physical PS5 UI/persistence testing is still required.

## v0.1.4 — Smooth coverflow and DualSense playback controls

- **L1 / R1**: switch artists in All Albums (unchanged). During playback, previous/next song (unchanged).
- **L2 / R2**: previous/next album jacket in All Albums (replaces the old header-tab shortcut).
- **Left/right stick and D-pad**: optional single-step album browsing; a held direction must return to neutral before triggering another change. Triggers use press edges, so holding them does not rapidly skip albums.
- **480ms cubic smoothstep coverflow**: sleeves translate and scale in/out of the center, with a synchronized album-background crossfade and focus glow. Additional cover switches are ignored during the animation.
- **X in All Albums**: starts the selected album at track 1. **X again during playback** opens the album's Tracklist, without pausing music. In Tracklist, X plays the highlighted track. Circle from Tracklist returns to playback.
- **Square**: play/pause toggle when a song is active, not Settings.
- **OPTIONS button**: Settings, and pressing OPTIONS again returns to the previous screen. Square no longer opens Settings.
- **Triangle**: Favorite toggle remains available in the album view, track list and playback.
- The five top tabs remain accessible through **Up** from All Albums, then D-pad Left/Right + X. Header tab browsing no longer uses L2/R2.

This is a PS5 homebrew test build; host animation tests and native package checks cannot substitute for visual testing on a PS5.

## v0.1.5 — New `.album` packed album format

PSJukebox now recognizes a ZIP archive renamed to **`.album`** in the **Artist folder**, alongside normal album folders and loose songs. The uploaded Tupac sample uses a flat ZIP archive with `cover.jpg`, `background.png`, 12 MP3 songs and incidental XML/SQLite metadata. Both flat archives and ZIP files with a single wrapping folder are supported.

```text
Music/
  Tupac/
    logo.png
    All Eyez on Me/
      cover.jpg
      song.mp3
    2pac-the-don-killuminati-the-7-day-theory.album
```

A packed album is loaded in the background on library scan. The existing read-only PS5 USB helper enumerates `.album` files too. The app stages the archive in its local cache when it is on USB, then extracts **only supported audio and `cover/background.png/jpg/jpeg`** into `/data/PSJukebox/AlbumCache`. It builds an album listing and plays the extracted songs with the existing FFmpeg decoder. Artist logo and `theme.ini` still come from the Artist folder, and the packed album's background follows normal album switching. Unsupported files (`.xml`, `.sqlite`) are ignored.

The cache avoids re-transferring and unpacking identical source path+archive size; replacing an archive with different contents but exactly the same byte size may require clearing `/data/PSJukebox/Cache/` and `/data/PSJukebox/AlbumCache/`. Requires writable local storage; large packed albums consume space for the archive **and** extracted songs. To reduce risk from malformed archives: ZIP members are streamed and CRC-checked, path traversal/symlinks are blocked, and limits are 1 GiB per packed file, 2 GiB total extracted, 512 MiB per song and 12 MiB per image. ZIP64/encrypted archives are not supported in this version.

A hardware test on the target PS5 remains necessary.

## v0.1.6 — Unsorted root .album packages and native SMB share playback

A `Music/*.album` package can be dropped **directly into the root Music folder**, with an internal file named **`<Artist>.artist`**, e.g. `Tupac.artist`. The marker may be empty; only the **filename** determines the artist. Existing `Music/Artist/*.album` files remain supported and do **not** require a marker. The marker can be at archive root or in a single wrapper folder. Invalid or missing markers in root archives are reported as `ALBUM_ARTIST_ERROR` in the app log, and do not cause empty/Unknown artists.

```text
Music/
  2pac-the-don-killuminati-the-7-day-theory.album
  wu-tang/
    logo.png
    enterthewutang/
      cover.jpg
      song.mp3

Inside 2pac-the-don-killuminati-the-7-day-theory.album:
  Tupac.artist                  <- empty file allowed, supplies artist identity
  cover.jpg
  background.png
  01 - Bomb First.mp3
  02 - Hail Mary.mp3
```

**Tracklist shortcut:** While the album cover carousel is selected, press **Down (D-pad or analog)** to open that album's Tracklist. Circle goes back. X still plays normally.

### SMB2/SMB3 share playback (read-only)

Create `/data/PSJukebox/smb.ini` using the PS5 file manager, then press Rescan in Settings. Configure a Windows/NAS SMB2 or SMB3 share with its Music root (or set the path to an existing Music subfolder):

```ini
server=192.168.1.50
share=Music
path=
username=jukebox
password=YOUR_SMB_PASSWORD
domain=
```

If your share exposes a top-level `Music` folder, use `share=Media` and `path=Music`. `domain` is optional; omit username/password only if the share permits Guest. No network credentials are ever placed in logs or GitHub, and the config is user-managed. Restrict access to `smb.ini` because it contains plaintext SMB credentials.

PSJukebox uses the PS5-native `libsmb2` SMB2/3 client. It scans artist directories and album/track names over the network; covers and songs are downloaded **on demand** to local PS5 cache, with pause/favorites/tracklist features unchanged. SMB .album packages are copied to cache and imported during library scan (so large archive imports may take time). No SMB upload, rename or delete actions are implemented. SMB connections require LAN reachability and firewall port 445, and the PlayStation homebrew sandbox must allow network sockets.

**Limits:** Read-only SMB share, up to one configured share, maximum 1 GiB per media/archive. Playback uses a local disk cache (network data is read on demand rather than streamed sample-by-sample into FFmpeg). PS5 hardware testing and actual SMB-server compatibility remain necessary.

## v0.1.7 — Fix Music-root .album files not appearing

The original helper used by v0.1.5 can remain resident when a newer app launches. Both old and new versions previously used the same port and JBOX1 handshake, so the old helper could silently omit Music-root `.album` packages from the catalog. **v0.1.7 moves the read-only helper to port 9189 and requires a JBOX2 response**, ensuring old helper instances can never be mistaken for the current one. It launches the bundled matching ELF over the existing elfldr service (:9021).

During scan, the app now writes `ALBUM_SCAN`, `ALBUM_FOUND`, `ALBUM_TRANSFER_OK`, `ALBUM_EXTRACT_BEGIN`, `ALBUM_OK` and detailed `ALBUM_TRANSFER_ERROR` / `ALBUM_INVALID` events to `/data/PSJukebox/psjukebox.log`. The helper reports oversized `.album` packages rather than silently ignoring them. Max supported package size remains 1 GiB.

Root packages with a valid `ArtistName.artist` file still appear under the correct artist; archives without any `.artist` marker now load under **Unsorted Albums** rather than disappearing, provided they contain valid supported music. Invalid/duplicate artist markers still produce an explicit error.

**Test:** install v0.1.7, keep `Music/wu-tang-forever.album` in place, open Settings / Rescan, then check `ALBUM_SCAN ... helper=...` and `ALBUM_OK ...` in the new log. Albums imported under Unsorted Albums can be repacked with `wutang.artist` later.

## v0.1.8 — Fix archive detection on PS5 sandbox
Actual PS5 logs from v0.1.7 show that the new helper finds `Music/wu-tang-forever.album` (109,407,657 bytes) and transfers it successfully, but the album reader subsequently fails with `Invalid packed album`. This is consistent with PS5 path metadata operations returning `errno=1`. The reader now validates packed file handles without `lstat` (uses `open` with `O_NOFOLLOW`, `fstat` and size fallback), and the extracted-library scanner tolerates path metadata denials for readable cache files. CI covers EPERM simulation for both `lstat` and `fstat`. If an album still fails, `ALBUM_INVALID` includes ZIP-parser or cache-specific detail.

## v0.1.9 — Native ZIP import crash fix / cleaner album header

An actual PS5 fault (`SIGSEGV` directly after `ALBUM_EXTRACT_BEGIN`) exposed excessive stack usage in the ZIP extraction routine: two 32 KiB local buffers on a background worker stack. v0.1.9 uses heap-owned scratch and includes a **64 KiB thread-stack regression test** for archive extraction, artist marker, cover/background and music scanning. The large album-directory name above the selected cover was also removed, leaving the carousel and playing track display intact. PS5 hardware acceptance remains necessary.

## v0.1.10 — Sandboxed .album cache indexing

The v0.1.9 console log showed the new packed album was found, copied and extracted, but the library displayed `ALBUM_OK ... tracks=0`. The PS5 sandbox refused `opendir` on app-owned AlbumCache paths (`errno=1`). Packed album files now **produce a complete Artist → Album → Track catalog directly from validated ZIP directory entries**. Audio paths, sizes, album covers, and album backgrounds are imported without scanning protected cache directories, including for already-extracted archive caches. CI tests simulate blocked `opendir` and verify playback paths can still be opened.

## v0.1.11 — Centered coverflow, spacing and live DualSense browsing

Cover jackets are centered vertically at y=525 rather than hanging lower in the screen. Adjacent jackets are spaced 380px center-to-center, leaving about 23.5px clear air between the focused jacket and each neighbor. The 480ms eased transitions, background image and reflection remain in place.

Controller actions are now consistent while a song is playing:
- **L1 / R1**: previous/next artist in the coverflow **without stopping playback**. Browsing automatically brings the carousel into view if you are on the player or tracklist page.
- **L2 / R2**: previous/next album cover, also while audio continues. The current song plays independently of the browsing selection.
- **D-pad LEFT / RIGHT**: jump backward/forward 10 seconds in the currently playing track; the cover does **not** change. No action if there is no active track.
- **Left or right joystick LEFT / RIGHT**: same 10-second seek, one jump per stick deflection (return to neutral to seek again).
- **D-pad DOWN or left/right joystick DOWN**: open the selected album's tracklist from the coverflow **or while the now-playing panel is visible**. If it is the playing album, highlight the currently playing track; Circle returns to playback.
- **X**: play the selected album/track, or open Tracklist from the player view.
- **Square**: pause/resume. **Triangle**: Favorites. **OPTIONS**: Settings.
- **UP on album view**: focus the header tabs (then Left/Right + X). Tracklist Up/Down still changes highlighted song.

The playing track metadata and progress continue updating while you browse other artists or albums. The play queue can advance automatically without forcing the visible browsing focus back to the playing album. No change to user music folders, .album files or SMB settings.

CI includes geometry/spacing, analog edge detection and live navigation wiring checks; confirm all controls and animation on PS5 hardware.

## v0.1.12 — D-pad coverflow / analog-only seeking

In the coverflow and now-playing views, **D-pad left/right** now selects the previous/next album cover with the existing 480ms easing and background transitions. **Either analog joystick left/right** seeks the currently playing song by -/+10 seconds, one step per deflection; returning to neutral rearms the next seek. D-pad no longer seeks, and analog left/right no longer changes album covers.

**L1/R1** still changes artists, **L2/R2** still changes albums, even during playback. **Down** opens the Tracklist, **Square** toggles Pause, **X** plays/opens tracks, **Triangle** toggles Favorites, **OPTIONS** opens Settings. D-pad left/right remains header tab navigation when header focus is active, so the header and artist/track lists retain their dedicated navigation.

This is an input-mapping change only. Packed `.album` scanning, cache extraction, SMB playback, centered jackets and their ~24px spacing are unchanged. Native CI tests validate that D-pad cover navigation and analog seeking are distinct paths; real PS5 behavior still needs a console test.

## v0.1.13 — Creator and build version in header

The upper-right PSJukebox header shows the existing clock with a small, right-aligned credit line directly underneath: **by YAHBALLON · v0.1.13**. The version displayed uses the native application's `version` constant, also used by its BOOT log, and the build workflow checks that it matches the `app.json` release version. The small muted label keeps the clock and other header UI unobstructed. No changes to music scanning, `.album` imports, playback, coverflow, buttons or SMB support.

## v0.1.14 — R2/L2 song navigation across album boundaries

**R2** plays the next song, **L2** plays the previous song, based on the current playback rather than whichever cover is being browsed. If the current song is the last track in an album, R2 starts **track 1 of the next album** under the same artist. If it's the first track, L2 starts the **last track of the previous album**. Albums without playable tracks are skipped, and the first/last album wrap cyclically within the same artist.

If music isn't playing yet, R2 starts the first track of the highlighted album and L2 starts its final track; empty albums are skipped.

On manual trigger navigation, the album cover and highlighted Tracklist song follow the new playing track, including USB/SMB staging. Auto-advance uses the same cross-album traversal but preserves unrelated browsing focus.

**L1/R1** still switch artists, **D-pad LEFT/RIGHT** changes album covers, analog sticks LEFT/RIGHT seek 10 seconds, **DOWN** opens the Tracklist, **Square** pauses and **OPTIONS** opens settings. Triggers also work from Favorites. Packed album files, SMB, themes and cache remain unchanged.

## v0.1.15 — Fast startup and progressive album discovery

PSJukebox now saves a **versioned, CRC-checked library snapshot** to `/data/PSJukebox/library-v3.cache` after a successful full discovery. Next boot, it reads this small catalog before starting the async local/USB/SMB refresh so artist selection, track lists and first album cover render immediately rather than waiting for the full rescanning of .album archives. This is an index containing title/path/artwork metadata only; it does not cache song bytes, SMB passwords or remote credentials.

On first use (or with a deleted/invalid index), the scan reports **partial libraries** as each packed album is found, rather than blocking all ten jackets until the last .album is processed. During a regular refresh, the current library and playback continue on screen; the fresh index replaces them only after discovery finishes. A corrupted/truncated cache is discarded safely, and the usual scan rebuilds it automatically.

Artwork fetching is prioritized: **current album cover first**, then artist logo and album background, then adjacent jackets (closest first). The UI can therefore show a cover before waiting on off-screen assets.

App-managed staged .album/audio cache reuse now tolerates the PS5 `EPERM` bug for stat/lstat/fstat (as seen on this console): the app can open and size an already downloaded file by file descriptor instead of copying a multi-hundred-megabyte ZIP from USB/SMB again. Existing staged data is reused only if its expected size matches, with symlink rejection.

Notes: The very first scan can still take time to stage a new .album; the snapshot accelerates subsequent startups. Files changed on disk are rechecked by the background scan. A complete rebuild may be initiated with Options → Rescan. New media and album covers appear progressively on a cold scan. Source .album files, user favorites, gamepad controls and playback logic are untouched.
