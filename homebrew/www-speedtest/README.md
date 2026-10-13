# WWW SPEEDTEST by YAHBALLON

Native PS5 homebrew internet connection tester, v0.4.2. Title ID: **PPSA99573**.

## Features
- Animated live-speed needle and selectable LibreSpeed country servers\n- Screenshot-inspired six-section dashboard and selection focus
- Persistent results history (up to 50 results) and auto-save toggle (session-only)
- Tabs for throughput graphs and ping/jitter information
- Cloudflare edge HTTPS download/upload throughput measurements
- Connection establishment latency and jitter (not ICMP ping)
- Live Mbps chart and 1920x1080 console UI
- Verified TLS using embedded CA bundle, network worker off the rendering thread
- No dummy speed readings or account requirement

**Limitations:** Cloudflare routes automatically; v0.4.2 does not guarantee availability of third-party regional test servers, IP/ISP lookup or reliable NAT detection. Results estimate application-layer throughput and are affected by TLS setup and server rate limits. Tests consume network data. Console hardware testing pending.

## Controller
D-Pad: menu navigation · Cross: run speed test on Speed Test view · Circle: back or exit · Triangle: home.

## Install
Download the released `PPSA99573.zip`, extract to `/data/homebrew/PPSA99573`, then register it with a compatible homebrew loader (e.g. ShadowMountPlus).

## Build
The dedicated root workflow `.github/workflows/build-www-speedtest.yml` clones the PS5-DevBridge simple-calc template, injects the native sources, prepares the PS5 OpenGL SDK and builds the installable ZIP.

## License
GPL-3.0-or-later, subject to upstream SDK/runtime licenses. Speed measurements target Cloudflare's public test endpoints, without embedding their proprietary UI or logo.

## v0.4.2
- Real payload-size accounting for LibreSpeed transfers.
- Country flags drawn natively for each server location.
- Regional servers remain third-party endpoints; reachability requires on-device verification.

## v0.4.2
- Fix sidebar trapping D-pad navigation while Server Select is open.
- Press X to enter server list, Circle to return to sidebar, and D-pad to access all other sections.

## v0.4.2
- Display actual local PS5 socket IP and remote server socket IP after connecting.
- Change Italy server to Bologna (requires on-device validation).
- Third-party India server remains unverified; failures are surfaced, never replaced with fake results.

## v0.4.2
- Retry Italy using three GARR endpoints in the same country.
- Show last network failure directly on the server selection and connection information screens.
- India remains unverified: failure is shown explicitly rather than reporting fictitious measurements.

## v0.4.2
- Add Mumbai, India OVHcloud HTTPS download benchmark using official public test file.
- Mumbai does not provide compatible HTTP upload tests: upload is N/A, and download-only results are not saved as complete history records.
- Preserve existing Bangalore, India LibreSpeed endpoint as an unverified legacy option.
