# WWW SPEEDTEST by YAHBALLON

Native PS5 homebrew internet connection tester, v0.2.0. Title ID: **PPSA99573**.

## Features
- Screenshot-inspired six-section dashboard and selection focus
- Persistent results history (up to 50 results) and auto-save toggle (session-only)
- Tabs for throughput graphs and ping/jitter information
- Cloudflare edge HTTPS download/upload throughput measurements
- Connection establishment latency and jitter (not ICMP ping)
- Live Mbps chart and 1920x1080 console UI
- Verified TLS using embedded CA bundle, network worker off the rendering thread
- No dummy speed readings or account requirement

**Limitations:** Cloudflare routes automatically; v0.2.0 does not offer cities as selectable test servers, IP/ISP lookup or reliable NAT detection. Results estimate application-layer throughput and are affected by TLS setup and server rate limits. Tests consume network data. Console hardware testing pending.

## Controller
D-Pad: menu navigation · Cross: run speed test on Speed Test view · Circle: back or exit · Triangle: home.

## Install
Download the released `PPSA99573.zip`, extract to `/data/homebrew/PPSA99573`, then register it with a compatible homebrew loader (e.g. ShadowMountPlus).

## Build
The dedicated root workflow `.github/workflows/build-www-speedtest.yml` clones the PS5-DevBridge simple-calc template, injects the native sources, prepares the PS5 OpenGL SDK and builds the installable ZIP.

## License
GPL-3.0-or-later, subject to upstream SDK/runtime licenses. Speed measurements target Cloudflare's public test endpoints, without embedding their proprietary UI or logo.
