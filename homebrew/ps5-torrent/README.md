# Torrent Flow for PS5

**Native PS5 BitTorrent test client by YAHBALLON — v0.1.1 — PPSA99684**

> Experimental first build. Not tested on physical PS5 hardware. Full protocol interoperability is not yet established.

Torrent Flow is an original console-friendly download dashboard backed by a compact native C++ BitTorrent v1 engine. The user interface is written for the native ps5-homebrew-ui renderer. [BitFiend](https://github.com/eduard-permyakov/BitFiend) was reviewed as protocol inspiration; its Linux-specific source is not incorporated.

## Features

- Five controller-accessible views: Dashboard, Torrent Library, Storage & Settings, Logs, About.
- HTTP BitTorrent tracker announce, Debian test-only UDP fallback via opentrackr, and IPv4 TCP peer connections.
- SHA-1 integrity verification per piece, independent of progress claims.
- Restart/resume by rehashing existing output file; cache includes a per-torrent resume snapshot.
- Persistent separate cache and download directories.
- Internal PS5 and optional USB0 root selection; idle-only switching.
- Bundled **Debian 13.7.0 amd64 netinst** legal torrent fixture, exactly **60,948 bytes**. Its SHA-256 is verified during the CI build.
- Live transferred bytes, downloaded speed estimate, piece progress, tracker and peer counts.

## Important known limitations

- Single-file BitTorrent v1 only. No multi-file torrents, magnet links, DHT, PEX, HTTPS or general-purpose UDP tracker support (only the Debian test fallback).
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

The .torrent fixture ships under assets/test and is loaded without entering a long URL on the controller.

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

The independent GitHub Actions workflow is stored at [build-ps5-torrent.yml](../../.github/workflows/build-ps5-torrent.yml). It fetches the native PS5 scaffold, copies this project's source, retrieves and SHA-256 checks the official Debian test torrent, compiles with the OpenGL SDK and OpenSSL, verifies a PS5 executable and uploads the installable ZIP.

Source files: native/main.cpp (UI), native/torrent_engine.cpp/.hpp (protocol engine).

The release workflow runs only on a successful push build to main. PR builds upload test artifacts but do not publish releases. Extract the generated PPSA99684.zip into /data/homebrew/ so /data/homebrew/PPSA99684/eboot.bin exists.

GPL-3.0-or-later; no BitFiend source code copied.

