# PS5 RandomX Benchmark by YAHBALLON v0.3.0

## PS5 hardware results already confirmed
- v0.1.0: **1.9862 H/s** (1 thread, software AES, light interpreter).
- v0.2.0: **7.8487 H/s** (4 threads, hardware AES, light interpreter, 236 hashes/30.069 sec).
- About **3.95x total throughput**, primarily attributable to 4 independent CPU workers. **The independent hardware-AES speedup is unmeasured**; compare the 1-thread software and hardware profiles.

## Experimental addition: full RandomX memory
- Fifth profile, **1 THREAD / FULL MEMORY**, manually selectable with Up/Down or L1/R1.
- RandomX allocates **about 2.08 GiB** for its dataset, plus a **256 MiB** cache during initialization. It may be unavailable on consoles with restrictive homebrew memory budgets.
- Single threaded, hardware AES when supported, **JIT disabled**.
- Dataset created through cancellable chunks from the same RandomX key; all items are initialized before benchmarking.
- Shows preparation progress; measures H/s only after initialization. The report also includes dataset byte count and initialization duration.
- Memory allocation failure results in an error instead of falling back silently. Other existing profiles remain unchanged.
- `Circle` or `Options` cancels a running preparation or hash benchmark. Initialization of the currently active chunk must complete before cancellation takes effect.
- Existing hardware-AES 1/2/4 worker profiles and the 1-thread software baseline are preserved.

## Important caveats
- The full-memory profile is **opt-in**, may use substantial RAM and could be rejected or terminated by the console. Do not run it alongside memory-heavy applications.
- Full-memory **without JIT** is not equivalent to a production RandomX miner, so it may not be significantly faster than light mode.
- No mining payouts, wallet, P2P connectivity or coin issuance.

Release: `ps5-randomx-v0.3.0`. Title ID: `PPSA99795`. Log: `/data/ps5-randomx/benchmark.log`.
