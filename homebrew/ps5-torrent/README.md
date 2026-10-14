# Torrent Flow for PS5

**Native PS5 BitTorrent test client by YAHBALLON — v0.2.0 — PPSA99684**

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
- Four concurrent PS5-compatible TCP peer workers with bitfield/HAVE/UNCHOKE negotiation and pipelined 16 KiB requests.
- Live transfer speed (smoothed), ETA, piece progress, tracker/peer counts, persistent diagnostic logs.
- Import additional safe single-file BitTorrent v1 metadata by copying `.torrent` files to `/data/ps5-torrent/cache` or `/mnt/usb0/ps5-torrent/cache` (then Triangle to refresh).

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

## Transfer engine v0.2.0

A new stateful wire-protocol engine fixes the common issue where peers shake hands successfully but no useful pieces are requested. It sends INTERESTED once, processes BITFIELD/HAVE/UNCHOKE, reserves only pieces the peer actually advertises, pipelines up to eight 16 KiB requests per piece, verifies each SHA-1 digest and persists it before advancing progress. Four workers schedule distinct pieces in parallel, with bounded timeouts and retry rounds. A duplicate/invalid peer can't mark a piece completed.

GitHub Actions now runs an **actual host-side C++ engine integration test**, using two localhost peers holding different pieces of the same ISO fixture, plus a tracker, corrupted-disk repair and verified resume.

## PS5 TCP connectivity fix (v0.1.3)

The prior versions used `fcntl(fd, F_SETFL, flags | O_NONBLOCK)`. Real PS5 logs showed this returned **EACCES / errno=13 for every single peer**, preventing any download. Torrent Flow now uses normal blocking TCP sockets with SO_SNDTIMEO and SO_RCVTIMEO set before connecting, matching the known working PS5 WWW Speedtest client. This avoids the forbidden `fcntl` path while preserving connection failure logs and verified torrent pieces.

The provided PS5 log confirmed that the Debian UDP tracker returned 64 peers, proving tracker discovery worked but connections could not start. Real PS5 transfer of v0.1.3 remains to be tested.

## Important known limitations

- Single-file BitTorrent v1 only. No multi-file torrents, magnet links, DHT, PEX, or peer encryption. HTTP/HTTPS/UDP trackers are supported; only IPv4 peers are used.
- Maximum four parallel peer connections. Peer discovery still relies on torrent trackers, so DHT-only swarms remain unsupported.
- Incoming connections, true seeding, upload speeds, pause/resume while open, and arbitrary torrent picker are not supported in the first build. SQUARE cancels the transfer and verified data remains on disk.
- Peer logic now respects CHOKE/UNCHOKE and requests pieces in a bounded pipeline; peer compatibility and complete download on an actual PS5 remain **unverified**.
- HTTP trackers transmit announce metadata unencrypted; only transfer legal, trusted content.
- SHA-1 integrity is the historical BitTorrent v1 piece hash and is not a replacement for authenticating a torrent's origin.

## Diagnostic logs

- **Fixed log folder:** `/data/ps5-torrent/logs/`
- **Active log:** `/data/ps5-torrent/logs/torrent.log`
- **Rotated log:** `/data/ps5-torrent/logs/torrent.log.old.log` (after about 1 MiB)
- Logs are never written to the installed app folder (`/data/homebrew/PPSA99684/logs/`). Old logs from prior versions remain untouched and are not migrated.
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

The six .torrent fixtures ship under assets/test and are selected with the controller. Advanced users may import other v1 single-file .torrent files via the cache folders; multi-file and magnet metadata are not supported.

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

Both host integration tests and PS5 compilation/package checks must succeed before the release workflow runs. The release workflow runs only on a successful push build to main. PR builds upload test artifacts but do not publish releases. Extract the generated PPSA99684.zip into /data/homebrew/ so /data/homebrew/PPSA99684/eboot.bin exists.

GPL-3.0-or-later; no BitFiend source code copied.

