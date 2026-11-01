# PS5 RandomX Benchmark by YAHBALLON v0.2.0

The first PS5 hardware report confirms **1.9845 H/s (30 s)** and **1.9862 H/s (60 s)** using RandomX Light mode, software AES, no JIT, one CPU worker. This release tests the next bottlenecks directly on the console.

## Changes
- Four real RandomX test profiles: 1 thread/software AES, and 1/2/4 threads/hardware AES.
- One shared 256 MiB light-mode cache; one independent VM and 2 MiB scratchpad per CPU worker.
- Runtime CPU AES capability check before any hardware-AES VM executes.
- Controller navigation: **Up/Down or L1/R1** chooses profile; Left/Right chooses duration; X starts; O cancels.
- Shared start barrier so measured H/s excludes cache/VM initialization; monotonic elapsed includes the last in-flight hash.
- Diagnostic log now writes **AES mode, thread count, profile, per-worker hash counts, elapsed, last digest and failure reason**.
- Stable Title ID `PPSA99795`; installer `PPSA99795.zip` for TESTBUILD.
- Fast mode (>2 GiB dataset), JIT, huge pages and real mining are intentionally **not enabled** until their safety and stability have been tested.

## Recommended PS5 tests
Run 30 or 60 seconds on these profiles in order: **1T software**, **1T hardware**, **2T hardware**, **4T hardware**. Do not assume linear speedup; thermal behavior, scheduling, cache contention and interpreter work can limit scaling.

Use the automatic log `/data/ps5-randomx/benchmark.log` to share all four results.

This remains a benchmark, **not a wallet, blockchain, miner or coin launch**.
