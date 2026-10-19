# MeshCloud by YAHBALLON

Security-first, cross-platform cooperative storage prototype for **PS5, Windows, Linux and macOS**.

## Rule 1: mandatory 10 GiB contribution

Every node must contribute at least **10 GiB (10 * 1024^3 bytes)** of **audited usable** storage before it can participate in **any network operation**. No exceptions: search, listings, uploads, downloads, metadata, replication and relays are all members-only. No guest or read-only mode.

The client checks locally for both a configured 10 GiB minimum and **12 GiB available on disk** (10 GiB contribution + 2 GiB emergency margin) as a preliminary safety check. This **does not prove remote storage**. Future membership requires peer-authenticated, expiring proof-of-storage audits. A self-reported number, sparse file or UI checkbox cannot qualify.

Until the peer identity / proof protocol is implemented and audited, all network endpoints remain disabled (fail closed). Desktop binaries from this first phase are local diagnostics, **not yet functioning MeshCloud peers**.

## Desktop builds

CMake C++20 project, built on Windows (MSVC), Linux (GCC), and macOS (AppleClang).

```sh
cmake -S homebrew/ps5-meshcloud -B build/meshcloud -DCMAKE_BUILD_TYPE=Release
cmake --build build/meshcloud --config Release
ctest --test-dir build/meshcloud -C Release --output-on-failure
```

Run `meshcloud --storage <directory> --quota-gib 10` to check local disk eligibility, or `meshcloud --help`. No network traffic is initiated.

GitHub Actions: `.github/workflows/build-meshcloud-desktop.yml` compiles/tests all desktop targets and publishes downloadable CI artifacts on push to main.

## Architecture

- Native C++ shared core, with PS5-DevBridge-based UI as a later platform adapter.
- Encrypted content-addressed blocks and signed manifests planned; AES-GCM keys must never be shared with storage providers.
- Peer authentication, short-lived membership credentials backed by randomly sampled storage challenges, anti-Sybil controls and rate limiting planned.
- Relay-only mode preferred to avoid exposing IPs to direct peers, although relays necessarily see connecting source IPs. **No absolute IP-anonymity claim**.
- DHT discovery and member-authenticated public metadata search planned. Private files excluded from public index.
- Reed-Solomon coding, independently placed fragments, recovery and automated repair planned.
- Secure updates, filesystem sandboxing, telemetry opt-in, abuse controls and security audits required before public launch.

### Honest project status

**v0.1.0 pre-alpha: desktop storage preflight + admission policy + tests/CI.**
Not a finished PS5 app, not yet a cross-platform P2P network, no real remote membership auditing or downloads.

## Research

- https://docs.libp2p.io/
- https://tahoe-lafs.readthedocs.io/
- https://github.com/Yahballon/PS5-DevBridge
