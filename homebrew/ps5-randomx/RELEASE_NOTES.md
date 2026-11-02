# PS5 RandomX Benchmark by YAHBALLON v0.6.2

## One-click SAFE 8 + PS5Coin progression

Real PS5 v0.6.1 has completed all 7 stable profiles with **147.92 H/s** at 4T Secure JIT. Full-Memory (2,080 MiB dataset) previously hung the console in v0.6.0, so the full dataset remains blocked in v0.6.2.

**Press X once on AUTO SAFE 8** (selected by default):
1. Light interpreter, software AES, 1 thread
2. Light interpreter, hardware AES, 1 thread
3. Light interpreter, hardware AES, 2 threads
4. Light interpreter, hardware AES, 4 threads
5. Light Secure JIT, 1 thread
6. Light Secure JIT, 2 threads
7. Light Secure JIT, 4 threads
8. **64 MiB RAM HEALTH CHECK**: separate, capped anonymous RW memory allocation; writes a deterministic marker to both ends of each 4-KiB interval, then reads and verifies every marker. Never creates a RandomX dataset or executes unknown code.

The RAM check does work in short chunks with cancellation checks and tiny sleeps to keep the UI responsive. Stages are appended (flushed) to `/data/ps5-randomx/memory_probe.log`, and results including checksum/checked bytes are appended to `benchmark.log`. The UI displays **RAM PASS** or **RAM FAIL** on the eighth row and preserves the 4-thread JIT H/s in the large headline.

**Limitations:** A successful 64 MiB probe does **not** prove 2.08 GiB full-memory availability. A console-level memory kill is still possible in principle, but this version never attempts that large allocation. The memory probe is an incremental diagnostic only.

### Actual project: PS5Coin

See `projects/ps5coin/README.md` for the concrete plan for an **independent P2P blockchain, desktop full nodes/wallets, and a native PS5 mining client** that uses the already confirmed RandomX Light JIT engine. A true low-difficulty testnet can be developed **without** full-memory mining. PS5-only mining cannot be enforced by ordinary decentralized PoW; the honest objective is PS5-first mining.

This is still a benchmark; no coins, rewards or network are created by this release. Native Title ID `PPSA99795`, release `ps5-randomx-v0.6.2`. GitHub Actions generates the TESTBUILD `version.json`.
