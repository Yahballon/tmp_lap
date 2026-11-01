# PS5 RandomX Benchmark by YAHBALLON

**v0.2.0 | Title ID: PPSA99795 | Native PS5 homebrew | Experimental**

A native console performance test using **real RandomX v1.2.1** with the same algorithm implementation used in the v0.1.0 successful test.

## Confirmed first hardware result (v0.1.0)
- Single worker, RandomX Light, interpreter, software AES, no JIT, ~256 MiB cache.
- 30 s: 60 hashes in 30.234 s = **1.9845 H/s**.
- 60 s: 120 hashes in 60.416 s = **1.9862 H/s**.
- The two numbers agree to within ~0.1%. They are **not** representative of Full Memory / JIT mining.

## v0.2.0 test matrix
- **1 thread / software AES**: regression baseline.
- **1 thread / hardware AES**: direct AES benefit.
- **2 threads / hardware AES**: scaling check.
- **4 threads / hardware AES**: scaling check with modest CPU usage.

All profiles use the **same shared, read-only RandomX light cache** (~256 MiB), but each worker has its own VM/scratchpad. Hardware AES is enabled **only when RandomX reports CPU support**. Fast memory mode (>2 GiB dataset), JIT, huge pages and network/mining features are still disabled.

## Controller
- **X**: Start selected test.
- **Up / Down or L1 / R1**: Choose the CPU profile.
- **Left / Right**: Choose a 10-, 30- or 60-second run.
- **O**: Stop the active run / exit when idle.
- **Triangle**: Clear displayed results.
- **Square**: Show the automatic log location.

## Benchmark timing and reporting
- Cache and per-VM initialization happen in background before the worker start barrier. They do **not** count toward the H/s measurement.
- Each worker stops after reaching the selected monotonic deadline; the final hash may finish after the selected duration.
- The displayed rate is total successful hashes divided by actual wall-clock seconds from release to final thread join, not the sum of each worker's CPU time.
- Results appended to `/data/ps5-randomx/benchmark.log` include selected profile, actual AES mode, worker count, hashes for each thread, last digest, duration, stop status, and errors.
- For comparable tests, run all four profiles at 30 or 60 seconds, then let the PS5 cool between repeated tests.

## Source + release
- Source: `homebrew/ps5-randomx/native/main.cpp`.
- Pin: `tevador/RandomX` v1.2.1 / `102f8acf90a7649ada410de5499a7ec62e49e1da`, BSD-3-Clause.
- Workflow: `.github/workflows/build-ps5-randomx.yml`.
- Installer: `PPSA99795.zip`; tag `ps5-randomx-v0.2.0`.
- `version.json` is generated **only** by the repository's catalog GitHub Action.
- No cryptocurrency network, wallet, payout or mining is launched.

The actual performance and runtime stability of the new profiles must be verified on a real console before considering them proven.

## Licensing
Homebrew glue: GPL-3.0-or-later. RandomX: upstream BSD 3-Clause, included in the package.
