# PSTUBE v0.1.0 (early development alpha)
**PSTUBE by YAHBALLON**, native PS5 homebrew using ps5-homebrew-ui and OpenGL.

## Current status
This is a **search-and-favourites alpha**, **not a working YouTube player**. The native UI, DualSense navigation, URL-encoded Invidious search, result detail view, favourite persistence, and offline/error states are implemented in source. Public Invidious instances can be unreliable. Thumbnail fetching, video decoding/audio playback, downloads, OAuth subscriptions, and YouTube account sync are **not implemented**. Controls never pretend that unsupported operations succeeded.

## Features
- Native fullscreen 1920x1080 UI, dark cyan design and branded header
- Live search through a configurable Invidious host (currently `yewtu.be`), HTTPS/TLS CA verification and 8-second socket timeouts
- Background search and capped HTTP response size; handle empty/error conditions
- Results with title, uploader and duration; local saved favourites
- Search keyboard with controller and a separate details screen
- Separate Home, Search, Trending, Favorites, Downloads, History and Settings navigation
- Honest status indicators for unavailable features

## Controls
| Control | Action |
| --- | --- |
| D-Pad | Navigate menus, results and keyboard |
| Cross | Select / type letter / show details |
| Circle | Back or exit |
| Triangle | Open search / submit typed query |
| Square | Favourite / backspace in keyboard |
| L1 / R1 | Change page |
| Left stick | Not yet enabled |
| Options | Not yet enabled |

## Metadata
- Title ID: `PPSA99762` (repository collision scan only)
- Version: `0.1.0`
- Release tag: `pstube-v0.1.0`
- Source: `homebrew/pstube/native`
- CI: `.github/workflows/build-pstube.yml`

## Build and installation
Run the dedicated GitHub Actions workflow, which stages PS5-DevBridge's simple-calc scaffold, embeds the system CA roots, builds via the PS5 OpenGL toolchain, and verifies the package.
After a **successful** CI build, extract `PPSA99762.zip` and copy `PPSA99762` to `/data/homebrew/`. Register with a compatible homebrew loader such as ShadowMountPlus. Only the real release ZIP is installable; source ZIPs are not.

Settings: `/download0/pstube.txt`, fallback `/data/pstube/state.txt` (schema 1). Favourites remain across app replacement. Query text and history currently persist for the session only.

## Security and limitations
Requests use certificate and hostname validation. The provider domain is not currently editable through the on-console settings screen. The HTTP client handles identity and chunked responses, but not compressed encoding or redirects; public instance behavior may vary. Video thumbnails, account login, stream format negotiation, playback and downloading require further native development. This package has not been validated on PS5 hardware.

GPL-3.0-or-later. Framework: [blackbearreloaded/ps5-homebrew-ui](https://github.com/blackbearreloaded/ps5-homebrew-ui). Video metadata from third-party Invidious instances; no affiliation with YouTube or Google.
