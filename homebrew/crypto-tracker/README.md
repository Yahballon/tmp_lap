# Crypto Tracker

Native PS5 cryptocurrency price tracker built on [BlackBearReloaded's ps5-homebrew-ui](https://github.com/blackbearreloaded/ps5-homebrew-ui), with the [PS5-DevBridge](https://github.com/Yahballon/PS5-DevBridge) build scaffold.

**Version:** v0.3.0 · **Title ID:** `PPSA99442` · **Content version:** `01.000.005`

## Features

- A native 1920×1080 dark-blue dashboard with cryptocurrency prices, sparklines and market-cap ranking.
- A scrollable multi-row Pinned section for all available saved favourites.
- Individually rendered coin icons from the CC0 [cryptocurrency-icons](https://github.com/spothq/cryptocurrency-icons) collection, bundled in an offline texture atlas, with coloured ticker fallbacks.
- Detail screen with market statistics and 1D, 7D, 30D, 90D, or 1Y charts.
- CoinGecko public HTTPS API, certificate validation and error reporting (including HTTP 429 limits).
- Persistent favourites saved in title-local settings.

## DualSense controls

| Button | Action |
| --- | --- |
| D-pad | Navigate the Home dashboard and chart ranges |
| L1 / R1 | Jump through rows in the Home dashboard |
| Cross | Open coin details |
| Circle | Back / Exit |
| Square | Pin or unpin the selected coin |
| Triangle | Refresh live prices |

## Repository location

All application-specific sources are located in `homebrew/crypto-tracker/native/`. The GitHub Actions workflow is at [`.github/workflows/build-ps5-crypto-tracker.yml`](https://github.com/Yahballon/tmp_lap/blob/main/.github/workflows/build-ps5-crypto-tracker.yml), because GitHub only discovers workflows from the repository-level workflows directory.

The workflow copies the native sources into a temporary PS5-DevBridge scaffold, fetches required SDK dependencies, generates app and coin icons, builds the FSELF and ZIP, and checks the contents. A successful first build of a new version can publish its ZIP to GitHub Releases; an already-published version is deliberately not overwritten.

**Download the existing v0.3.0 release:** [GitHub Release](https://github.com/Yahballon/tmp_lap/releases/tag/v0.3.0).

## Installation

1. Download `PPSA99442.zip` from a published release.
2. Extract `PPSA99442/` to `/data/homebrew/` on the PS5.
3. Register/refresh the app in ShadowMountPlus or your compatible launcher.

The versioned app folder and Title ID are unchanged by this repository reorganization; app compatibility still depends on your PS5 firmware, homebrew runtime, network setup and loader.

## License and notices

Application source uses GPL-3.0-or-later. Respect all notices and license obligations from the ps5-homebrew-ui scaffold and SDK dependencies. Cryptocurrency icon artwork is CC0 1.0 under the upstream project's terms.
