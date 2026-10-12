# PS5 Crypto Tracker

Native PlayStation 5 crypto market tracker built with **BlackBearReloaded's [ps5-homebrew-ui](https://github.com/blackbearreloaded/ps5-homebrew-ui)** via the PS5-DevBridge calculator-based scaffold. The source is GPL-3.0-or-later; preserve the original framework notices.

**Title ID:** `PPSA99442` · **App version:** `01.000.001`

## Screens

- **Home:** pinned Bitcoin/Ethereum/Litecoin cards followed by market-cap-ranked coins, 24-hour changes and 7-day sparklines.
- **Coin detail:** price, change, market cap, 24h volume, supply, ATH/ATL, live range chart (1D, 7D, 30D, 90D, 1Y).
- 1920×1080 virtual canvas scaling to the actual PS5 display.

## Controls

- **D-pad:** browse Home tiles / select detail chart range.
- **X:** open coin detail.
- **Circle:** back; exit from Home.
- **Triangle:** refresh live market quotes.

## Data and privacy

Fetches USD quotes directly from **CoinGecko's public REST API** with verified OpenSSL HTTPS. Uses the repository's pinned PacBrew OpenSSL libraries and a CA trust bundle copied from the Linux build runner. The app never invents prices: until the first successful request, it displays a loading/error message. Free CoinGecko endpoints can return HTTP 429 or restrict historical chart data. Refresh via Triangle after waiting. Charts show a clearly identified sparkline preview until range data loads. It does not require a hosted PC bridge, API token, account, or access to a wallet.

No buy/sell orders or wallet private keys.

## Build

Workflow: [Build PS5 Crypto Tracker](../../actions/workflows/build-ps5-crypto-tracker.yml). On a successful Actions run, download the `PS5-Crypto-Tracker-PPSA99442` artifact and unzip the inner `PPSA99442.zip`. Copy the `PPSA99442` folder to `/data/homebrew/` and register through your homebrew loader / ShadowMountPlus.

GitHub Actions checks for a valid FSELF header and required files, but **hardware functionality, the TLS/OpenSSL runtime and network access have not yet been tested on a real PS5**. Fix any compiler errors or network compatibility issues using the workflow logs.

## Original web project

The supplied React/Vite `cryptotracker.zip` was studied for design, API routes, and features. This is a real native C++ implementation, **not** an Electron/Chromium wrapper. No bundled web credentials are used.

## Licenses

The renderer and adapted scaffold derive from BlackBearReloaded's GPL-3.0-or-later project. See that upstream repository's LICENSE and THIRD_PARTY_NOTICES.md. The build copies its supporting font assets and licences.
