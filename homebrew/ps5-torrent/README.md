# Torrent Flow for PS5

**Native PS5 BitTorrent test client by YAHBALLON — v0.1.3 — PPSA99684**

> Experimental first build. Not tested on physical PS5 hardware. Full protocol interoperability is not yet established.

Torrent Flow is an original console-friendly download dashboard backed by a compact native C++ BitTorrent v1 engine. The user interface is written for the native ps5-homebrew-ui renderer. [BitFiend](https://github.com/eduard-permyakov/BitFiend) was reviewed as protocol inspiration; its Linux-specific source is not incorporated.

## Features

- Five controller-accessible views: Dashboard, Torrent Library, Storage & Settings, Logs, About.
- HTTP, certificate-verified HTTPS and UDP BitTorrent trackers (including announce-list fallbacks) and IPv4 TCP peer connections. Debian includes an optional OpenTrackr fallback.
- SHA-1 integrity verification per piece, independent of progress claims.
- Restart/resume by rehashing existing output file; cache includes a per-torrent resume snapshot.
- Persistent separate cache and download directories.
- Internal PS5 and optional USB0 root selection; idle-only switching.
- **Six bundled legal Linux torrents:** the original Debian 13.7.0 netinst plus five additional official Ubuntu and Linux Mint releases. GitHub Actions validates each bencoded single-file torrent's piece hashes, file length, and expected ISO name; the Debian metadata SHA-256 is pinned.
- The library displays readable titles and approximate ISO sizes, with D-Pad selection and X to download the chosen torrent.
- Live transferred bytes, downloaded speed estimate, piece progress, tracker and peer counts.

## Bundled test torrents

| Name | Approx. ISO size | Official metadata source |
| --- | --- | --- |
| Debian 13.7 netinst | 756 MiB | [Debian CD images](https://www.debian.org/CD/torrent-cd/) |
| Ubuntu 26.04.1 LTS Server | 2.7 GiB | [Ubuntu 26.04.1](https://releases.ubuntu.com/26.04/) |
| Ubuntu 24.04.5 LTS Server | 3.8 GiB | [Ubuntu 24.04.5](https://releases.ubuntu.com/24.04/) |
| Linux Mint 22.3 Cinnamon | 2.9 GiB | [Mint release post](https://blog.linuxmint.com/?p=4981) |
| Linux Mint 22.3 Xfce | 2.8 GiB | [Mint release post](https://blog.linuxmint.com/?p=4981) |
| Linux Mint 22.3 MATE | 2.9 GiB | [Mint release post](https://blog.linuxmint.com/?p=4981) |

Only .torrent metadata files are packaged. The ISO files themselves are downloaded over BitTorrent upon selection. Tracker/peer connectivity remains hardware-untested.

## PS5 TCP connectivity fix (v0.1.3)

The prior versions used `fcntl(fd, F_SETFL, flags | O_NONBLOCK)`. Real PS5 logs showed this returned **EACCES / errno=13 for every single peer**, preventing any download. Torrent Flow now uses normal blocking TCP sockets with SO_SNDTIMEO and SO_RCVTIMEO set before connecting, matching the known working PS5 WWW Speedtest client. This avoids the forbidden `fcntl` path while preserving connection failure logs and verified torrent pieces.

The provided PS5 log confirmed that the Debian UDP tracker returned 64 peers, proving tracker discovery worked but connections could not start. Real PS5 transfer of v0.1.3 remains to be tested.

## Important known limitations

- Single-file BitTorrent v1 only. No multi-file torrents, magnet links, DHT, PEX, or peer encryption. HTTP/HTTPS/UDP trackers are supported; only IPv4 peers are used.
- Single peer connection at a time; transfer throughput and swarm availability may be limited.
- Incoming connections, true seeding, upload speeds, pause/resume while open, and arbitrary torrent picker are not supported in the first build. SQUARE cancels the transfer and verified data remains on disk.
- Peer logic now respects CHOKE/UNCHOKE and requests pieces in a bounded pipeline; peer compatibility and complete download on an actual PS5 remain **unverified**.
- HTTP trackers transmit announce metadata unencrypted; only transfer legal, trusted content.
- SHA-1 integrity is the historical BitTorrent v1 piece hash and is not a replacement for authenticating a torrent's origin.

## Diagnostic logs

- Primary log folder: `/data/homebrew/PPSA99684/logs/`
- Active log: `/data/homebrew/PPSA99684/logs/torrent.log`
- Rotated log: `torrent.log.old.log` (after about 1 MiB)
- If the installed app folder is not writable, fallback: `/data/ps5-torrent/logs/torrent.log`
- The **Logs** tab shows the exact active log path and most recent log lines.
- Logs include network addresses and should be reviewed/redacted before publishing publicly.
- Statuses include DNS errors, tracker responses, socket timeouts, peer handshakes, and piece hash checks.

## Default directories

Internal:
- Cache: /data/ps5-torrent/cache
- Downloads: /data/ps5-torrent/downloads

USB0:
- Cache: /mnt/usb0/ps5-torrent/cache
- Downloads: /mnt/usb0/ps5-torrent/downloads

The six .torrent fixtures ship under assets/test and are selected with the controller; no typing or manual USB import is required.

## Controller

| Input | Action |
| --- | --- |
| D-Pad | Select list items / navigate |
| X | Start bundled torrent / toggle storage |
| Circle | Go back or exit |
| Square | Request transfer stop |
| Triangle | Refresh torrent library |
| L1/R1 | Switch pages |

## Build

The native client uses a PEM trust bundle built into the executable for HTTPS tracker certificate and hostname validation. Torrent metadata SHA-256 is pinned for the six official test torrents.

The independent GitHub Actions workflow is stored at [build-ps5-torrent.yml](../../.github/workflows/build-ps5-torrent.yml). It fetches the native PS5 scaffold, copies this project's source, retrieves and SHA-256 checks the original Debian test torrent, fetches five new official test torrents and verifies their single-file v1 metadata, compiles with the OpenGL SDK and OpenSSL, verifies a PS5 executable and uploads the installable ZIP.

Source files: native/main.cpp (UI), native/torrent_engine.cpp/.hpp (protocol engine).

The release workflow runs only on a successful push build to main. PR builds upload test artifacts but do not publish releases. Extract the generated PPSA99684.zip into /data/homebrew/ so /data/homebrew/PPSA99684/eboot.bin exists.

GPL-3.0-or-later; no BitFiend source code copied.

