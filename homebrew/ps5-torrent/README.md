# Torrent Flow for PS5

**Native PS5 BitTorrent test client by YAHBALLON — v0.3.2 — PPSA99684**

> Experimental first build. Not tested on physical PS5 hardware. Full protocol interoperability is not yet established.

Torrent Flow is an original console-friendly download dashboard backed by a compact native C++ BitTorrent v1 engine. The user interface is written for the native ps5-homebrew-ui renderer. [BitFiend](https://github.com/eduard-permyakov/BitFiend) was reviewed as protocol inspiration; its Linux-specific source is not incorporated.

## Features

- Six controller-accessible views: Dashboard, Torrent Library, Downloads, Settings, Logs, About.
- HTTP, certificate-verified HTTPS and UDP BitTorrent trackers (including announce-list fallbacks) and IPv4 TCP peer connections.
- SHA-1 integrity verification per piece, independent of progress claims.
- Restart/resume by rehashing existing output file; cache includes a per-torrent resume snapshot.
- Persistent separate cache and download directories.
- Internal PS5 and optional USB0 root selection; idle-only switching.
- **Six refreshed legal desktop Linux release torrents** from official Ubuntu and Fedora sources. GitHub Actions validates their bencoded single-file metadata, piece hash count, expected image name and size, and pins downloaded metadata hashes after first build.
- The library displays readable titles and approximate ISO sizes. X queues the selected torrent; SQUARE queues all torrents (Download All).
- Eight concurrent PS5-compatible TCP peer workers with bitfield/HAVE/UNCHOKE negotiation and pipelined 16 KiB requests.
- Live transfer speed (smoothed), ETA, piece progress, tracker/peer counts, persistent diagnostic logs.
- Import additional safe single-file BitTorrent v1 metadata by copying `.torrent` files to `/data/ps5-torrent/cache` or `/mnt/usb0/ps5-torrent/cache` (then Triangle to refresh).

## Auto-download cancel safety (v0.3.1)

Cancelled or failed auto-imported jobs remain in the Downloads tab and will **not** be restarted by the directory scanner, even while `auto_download=1`. Their original `.torrent` files remain available. Only an explicit Retry on the Downloads tab or another manual enqueue will restart them. This is exercised by the CI local swarm tests.

## Downloads tab and queue (v0.3.0)

- Every queued or active download appears separately in the sidebar's **Downloads** tab with individual progress, active peers, speed and status.
- **X** in Library queues one torrent. **SQUARE** in Library queues all available torrents without duplicating jobs. Jobs are persisted across restarts in `/data/ps5-torrent/cache/queue.db`.
- **X** in Downloads retries a failed/cancelled torrent; **SQUARE** stops only the selected torrent. Dashboard **SQUARE** stops all active jobs.
- Each active download has its own sockets and cancellation state. Up to three torrents can run concurrently (default **2**).
- Stop or application exit keeps verified file data. Restarted unfinished jobs rehash their stored data before resuming.
- True simultaneous torrents require independent TCP sessions. Each torrent keeps its eight-peer pool, so increasing parallel downloads also increases PS5 network/socket usage.

## Auto-download and settings.ini

The application creates these paths:

```ini
# /data/ps5-torrent/settings.ini
[downloads]
auto_download=0
max_active_downloads=2
delete_completed_torrents=1
scan_interval_seconds=3
storage=internal
```

Copy **complete** `.torrent` files into `/data/ps5-torrent/torrents/`. When `auto_download=1`, Torrent Flow scans this directory (default every 3 seconds) **while the app is running**, waits for a file to have unchanged size/mtime across two scans, validates its single-file v1 metadata and queues it. It keeps processing pending jobs until the queue is empty. This is **not a background system daemon**, so the PS5 app needs to stay open.

After a fully SHA-1 verified successful download, the corresponding **user-supplied** `.torrent` file from the watched `torrents/` or imported `cache/` folder is deleted if `delete_completed_torrents=1`. On errors, incomplete downloads, or cancel, the metadata is retained. Bundled `.torrent` files in `/app0/assets/test/` are **never deleted**.

Settings can be changed using the PS5 Settings tab (X on highlighted row), or by editing `settings.ini` before launching. Download data lives in `/data/ps5-torrent/downloads/` by default.

## Bundled test torrents (v0.3.2)

| Distribution | Approximate ISO size | Official torrent directory |
| --- | --- | --- |
| Ubuntu 26.04.1 LTS Desktop | 6.0 GiB | [Ubuntu 26.04](https://releases.ubuntu.com/26.04/) |
| Ubuntu 24.04.5.1 LTS Desktop | 5.8 GiB | [Ubuntu 24.04](https://releases.ubuntu.com/24.04/) |
| Fedora Workstation 44 | 2.7 GiB | [Fedora torrents](https://torrent.fedoraproject.org/) |
| Kubuntu 26.04.1 LTS | 4.7 GiB | [Kubuntu releases](https://cdimage.ubuntu.com/kubuntu/releases/26.04/release/) |
| Xubuntu 26.04.1 LTS | 4.8 GiB | [Xubuntu releases](https://cdimage.ubuntu.com/xubuntu/releases/26.04/release/) |
| Lubuntu 26.04.1 LTS | 3.7 GiB | [Lubuntu releases](https://cdimage.ubuntu.com/lubuntu/releases/26.04/release/) |

These are official, freely redistributable installer images. They were selected for wider distribution and more promising swarm availability, **not on verified real-time seeder counts**. Current peer counts vary with time, tracker and network. The PS5 client's limited protocol support also affects actual download performance.

Only .torrent metadata files are packaged. The ISO files themselves are downloaded over BitTorrent upon selection. Tracker/peer connectivity remains hardware-untested.

## Faster cancellation and connection handling (v0.3.0)

- Stop/Circle now marks cancellation once and shuts down all registered tracker and peer sockets, which wakes blocked recv/send and prevents repetitive Stop log spam.
- A cancellable TCP connect uses `ioctl(FIONBIO)` and `select()` with 200 ms cancellation checks. If that is unavailable, `SOCK_NONBLOCK` is attempted. The known-good PS5 blocking socket remains a fallback when both are rejected, and **this fallback can still stall until the kernel's connect timeout**; logs identify the mode actually used.
- Up to eight peer workers (previously four), shuffled peer order per tracker round, Fast Extension negotiation for peers advertising `HAVE_ALL`, and up to sixteen outstanding 16 KiB blocks per peer.
- Resume snapshots are written in batches to reduce tiny disk writes; every downloaded piece is still SHA-1 checked and flushed before recording progress.
- The host-side CI integration test now verifies that Cancel interrupts a deliberately silent peer, as well as full download, corrupted-piece repair, and resume.
- Real PS5 results are still needed to confirm whether FIONBIO or SOCK_NONBLOCK is allowed on your firmware.

## Transfer engine v0.2.0

A new stateful wire-protocol engine fixes the common issue where peers shake hands successfully but no useful pieces are requested. It sends INTERESTED once, processes BITFIELD/HAVE/UNCHOKE, reserves only pieces the peer actually advertises, pipelines up to eight 16 KiB requests per piece, verifies each SHA-1 digest and persists it before advancing progress. Eight workers schedule distinct pieces in parallel, with bounded timeouts and retry rounds. A duplicate/invalid peer can't mark a piece completed.

GitHub Actions now runs an **actual host-side C++ engine integration test**, using two localhost peers holding different pieces of the same ISO fixture, plus a tracker, corrupted-disk repair and verified resume.

## PS5 TCP connectivity fix (v0.1.3)

The prior versions used `fcntl(fd, F_SETFL, flags | O_NONBLOCK)`. Real PS5 logs showed this returned **EACCES / errno=13 for every single peer**, preventing any download. Torrent Flow now uses normal blocking TCP sockets with SO_SNDTIMEO and SO_RCVTIMEO set before connecting, matching the known working PS5 WWW Speedtest client. This avoids the forbidden `fcntl` path while preserving connection failure logs and verified torrent pieces.

The provided PS5 log confirmed that the Debian UDP tracker returned 64 peers, proving tracker discovery worked but connections could not start. Real PS5 transfer of v0.1.3 remains to be tested.

## Important known limitations

- Single-file BitTorrent v1 only. No multi-file torrents, magnet links, DHT, PEX, or peer encryption. HTTP/HTTPS/UDP trackers are supported; only IPv4 peers are used.
- Maximum eight parallel peer connections. Peer discovery still relies on torrent trackers, so DHT-only swarms remain unsupported.
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

