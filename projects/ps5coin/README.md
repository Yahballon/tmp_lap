# PS5Coin · PS5-first decentralized proof-of-work currency

**Development status: protocol blueprint, not a launched cryptocurrency.**
**Maintainer: YAHBALLON.** Homebrew client header: `PS5COIN MINER by YAHBALLON`.

## The actual goal

Build an independent cryptocurrency network in which jailbroken PS5 consoles can participate as **real mining clients**, earn genuine testnet/mainnet block rewards once the chain is launched, and hold/transfer coins using a wallet. The network must remain operational without a central website or server (P2P full nodes).

This is **not** the same as the RandomX benchmark or a token on somebody else's chain.

## Confirmed console capabilities

- PS5 homebrew successfully computes real RandomX v1.2.1 hashes.
- Light/Interpreter, 4 workers: approximately 7.8–8.1 H/s.
- Light/Secure JIT, 1/2/4 workers: approximately 39.8/77.1/147.9 H/s on real hardware.
- JIT source-code mapping fixed by moving an exact assembly copy from X-only `.text` to readable `.rodata`.
- Hash correctness compares a JIT digest with an interpreter digest before benchmark timing.
- Full Memory (2,080 MiB dataset + 256 MiB cache + VM scratchpads) caused a **console-wide freeze** and is **currently disabled**. Do not equate Light-JIT measurements with optimized full-memory miner performance.
- The v0.6.2 safe memory probe tests only 64 MiB to collect allocation/page-touch evidence without using a dataset.

## Codebase choice

1. **Blockchain/full-node starting point:** `monero-project/monero` (research fork), selected because it already integrates RandomX PoW, P2P networking, transaction validation, wallets, block templates and difficulty adjustment. Use a distinct chain identity, genesis block, network UUIDs, address prefixes and testnet ports. Keep upstream licensing.
2. **Consensus PoW:** RandomX with a network-defined seed/key schedule and a canonical block hashing/serialization implementation shared by the full nodes. Simply submitting the benchmark hash is **not** a valid block!
3. **PS5 miner:** Native C/C++ `PS5COIN MINER by YAHBALLON`, using our verified PS5 RandomX JIT implementation, network block templates, nonce search and honest difficulty/target checking. Console must be able to stop mining; settings and error logs persist in `/data/PS5Coin/`.
4. **Desktop node + wallet:** Windows/Linux/macOS reference full nodes and a secure desktop wallet first. PS5 wallet should initially be watch-only until private-key management on a jailbroken console has been reviewed.
5. **P2P bootstrap:** Initial seed addresses help peers discover one another but are **not** block authorities. Any peer can validate blocks and relay transactions without the bootstrap servers remaining online.

## Proposed **testnet** economics (NOT implemented)

- Symbol `P5C`; testnet chain ID and genesis yet to be generated.
- Target block spacing: 120 seconds.
- Proposed fixed-cap issuance: 21,000,000 P5C, zero premine, reward 10 P5C per block with halvings every 1,050,000 blocks. These parameters **would require implementing and independently testing changes to upstream monetary consensus**. They are not inherited from Monero.
- Transaction fees, coinbase maturity, address encoding, unlock rules, difficulty algorithm and fork activation are **unfinalized**. Never run a mainnet or distribute coins until they are specified, tested and independently reviewed.

## Milestones and acceptance tests

| Phase | Deliverable | Acceptance |
| --- | --- | --- |
| B0 | Native RandomX PS5 benchmark | **Done**: Secure JIT, real hashes, 1/2/4 workers |
| B1 | Console RAM health diagnostics | **v0.6.2 planned**: safe 64 MiB page-touch, reusable results |
| T0 | Standalone PC testnet full node | Independent genesis, separate network identity, node sync between at least two PCs |
| T1 | PC testnet wallet + transactions | Correct address/key handling, signed transfer, synchronization and reorg tests |
| T2 | PS5 miner connected to PC node | Receive canonical block template, compute PoW, submit an accepted block; reproducible difficulty checks |
| T3 | Distributed PS5 + PC testnet | At least two independent nodes and two PS5 miners; no permanent centralized coordinator |
| T4 | Persistent PS5 wallet and installable miner | Controller UI, crash-recovery, opt-in mining, logging, TESTBUILD and JB Store catalog |
| R0 | Security and consensus review | Network can withstand malformed blocks, forks, reorgs and DoS before any mainnet |

**Priority after the RAM diagnostic: T0 + T2 in parallel.** The existing 148 H/s Light JIT engine is suitable for a **low-difficulty private testnet mining prototype now**. Full-memory mode is an optimization, not a prerequisite to implementing a genuine proof-of-work miner and node interface.

## Critical limitation — PS5-only mining

A permissionless PoW network **cannot prove the hardware that found a nonce**. A PC or ASIC can run or emulate the same mining function. An untrusted self-reported `device=PS5` field is not security. This project is therefore explicitly **PS5-first, not provably PS5-exclusive**. Trusted hardware attestation would introduce major compatibility/decentralization tradeoffs, particularly with jailbroken consoles.

## Security rules

- No claims of real balances/rewards before an actual independently validated network exists.
- No mainnet keys or user funds on experimental jailbroken builds.
- No hardcoded mining account, stealth miner, background mining by default, or private-key logging.
- Full Memory disabled following the freeze until separate memory safety/availability work proves it viable.
- `version.json` for TESTBUILD is created by GitHub Actions, **never edited by hand**.

### Next repository layout (proposal)

```text
PS5Coin/
  node/           # upstream Monero-derived chain fork, independent genesis
  wallet/         # desktop testnet wallet initially
  miner-ps5/      # native homebrew app, JIT miner, TESTBUILD workflow
  tests/          # consensus, PoW vectors, peer-network, wallet, reorgs
  docs/           # protocol, security, chain params, mining RPC
```

This document is a **concrete plan**, not a working PS5Coin network. The RandomX benchmark is already real and is the engine to reuse.
