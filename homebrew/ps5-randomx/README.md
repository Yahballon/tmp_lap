# PS5 RandomX Benchmark by YAHBALLON

**v0.1.0 | Title ID: PPSA99795 | Native PS5 homebrew | Experimental**

This first-stage PS5Coin research tool measures **real RandomX v1.2.1 hashes per second on the PS5** using the upstream open-source library. It is not a wallet, a coin, a blockchain or a network miner.

## Controls
- **X**: Start the benchmark
- **Left/Right or L1/R1**: Choose 10, 30 or 60 seconds
- **O**: Cancel running test, or exit when idle
- **Triangle**: Reset on-screen results
- **Square**: Show the report location

## Execution
- RandomX **Light mode** (~256 MiB cache), single CPU thread.
- No JIT, no huge pages, software AES for a conservative baseline.
- Starts with CPU cache setup in a worker thread so the controller UI remains interactive.
- H/s includes only hash processing time, not initial cache creation.
- Never connects to peers or generates cryptocurrency.
- Does not automatically mine in the background.
- Writes report after a completed/cancelled run to `/data/ps5-randomx/benchmark.log`; write may be unavailable depending on mounted permissions.
- Early builds have not been verified on a physical PS5. Use a jailbroken testing system and monitor thermals.

## Source + release
- Source: `homebrew/ps5-randomx/native/main.cpp`.
- Native cross-building pulls **pinned** `tevador/RandomX` v1.2.1 (`102f8acf90a7649ada410de5499a7ec62e49e1da`) and statically links the actual library.
- GitHub Action: `.github/workflows/build-ps5-randomx.yml`.
- Expected installer: `PPSA99795.zip`.
- On a successful main branch build the action publishes `ps5-randomx-v0.1.0`. The existing catalog action manages `version.json`; do not edit it manually.

## Important caveat
The RandomX light-mode result is not the expected hashrate for full memory/JIT mining. We will use this measurement to determine whether and how to add multi-thread and fast-mode mining safely. PS5-only mining cannot be proven by a shared proof-of-work function alone.

## Licensing
PS5 homebrew integration: GPL-3.0-or-later. RandomX © upstream contributors, BSD 3-Clause; see the official upstream license copied into the installer at build time.
