# PS5Coin RandomX job protocol — LOCAL DEVELOPMENT PROTOTYPE

**Maintainer:** YAHBALLON · **Stage:** Dev T0/T2 protocol interoperability groundwork

This is a **real RandomX proof-of-work job generator and validator**, not a finished blockchain, P2P network, wallet, or coin. It reuses unmodified upstream RandomX v1.2.1 on a Windows/Linux/macOS reference host, and defines the work message a future native PS5 miner needs to speak.

## Test locally

First build the pinned RandomX dynamic library, on Linux:

```bash
git clone --depth=1 --branch v1.2.1 https://github.com/tevador/RandomX.git /tmp/p5c-randomx
cmake -S /tmp/p5c-randomx -B /tmp/p5c-randomx-build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
cmake --build /tmp/p5c-randomx-build -j4 --target randomx
export RANDOMX_LIB=/tmp/p5c-randomx-build/librandomx.so
```

Terminal A:

```bash
python3 projects/ps5coin/testnet/job_node.py --host 127.0.0.1 --port 19444 --difficulty-bits 4
```

Terminal B:

```bash
python3 projects/ps5coin/testnet/reference_miner.py --url http://127.0.0.1:19444 --miner reference_pc
```

The reference miner requests a job, computes true RandomX hashes (light/interpreted), and submits a nonce. The node independently recomputes and verifies the PoW target. A successful submission advances **the local in-memory prototype chain tip** (no disk ledger, P2P synchronization, balances or rewards). Restarting this demo resets the tip.

**Do not expose this server to the public internet.** It has no auth, throttling, persistence, peer networking or DoS protections. Its default bind is loopback.

## Experimental HTTP protocol v0.0.1

- `GET /status` gives prototype chain height, tip and warnings
- `GET /job?miner=<alphanumeric_id>` returns `job_id`, `header_prefix_hex`, `nonce_encoding=uint64_le`, `randomx_seed_hex`, `target_hex`, height and time to expiry
- `POST /submit` with `{"job_id":"<id>","nonce":123}` verifies full RandomX hash and returns `accepted`, `block` or a rejection reason

Header bytes are **exactly 92 bytes**: 4-byte magic `P5T1`, 8-byte height (unsigned LE), 8-byte seconds timestamp (unsigned LE), 32-byte previous digest, 32-byte SHA256 digest of the miner label, followed by an 8-byte unsigned LE nonce.

For this *development protocol only*, interpret the 32-byte RandomX output as a **big-endian unsigned 256-bit integer** and accept if `hash <= target`; `target = 2**(256 - difficulty_bits) - 1`. The node uses a fixed, non-mainnet seed and trivially low test difficulty; no live financial or chain consensus security.

The C++17 `../miner-ps5/pow_wire.hpp` encodes the same nonce and target rule and is checked in CI. It does **not** yet contain the native PS5 HTTP client, RandomX miner integration or a working PS5-installable PS5Coin Miner.

## Automated tests

`.github/workflows/test-ps5coin-job-protocol.yml` compiles the original pinned RandomX as a shared library, runs a C++17 PS5 wire-codec test, runs Python end-to-end hashing tests that prove accepted/rejected solutions with REAL RandomX, and exercises the local HTTP `/job`, `/submit`, and `/status` endpoints.

## What remains before claiming an actual cryptocurrency

1. Independent genesis and chain parameters, validated cryptographic consensus, difficulty retargeting, fork/reorg handling, inflation/reward and transaction rules
2. Durable full-node ledger, cryptographically signed transfers and wallet key handling
3. Robust authenticated/filtered peer discovery and decentralized P2P propagation
4. Portable native PS5 socket client, signed/checksummed work framing where appropriate, nonce iteration using the already working PS5 secure-JIT code, and accepted-block proof against an independent PC node
5. Network abuse-resistance and independent security/consensus testing

**PS5-only mining cannot be reliably enforced by ordinary PoW**. The target is a decentralized **PS5-first** network.
