# MeshCloud by YAHBALLON

**v0.3.0 pre-alpha – encrypted local vault, CIDv1, Reed-Solomon redundancy and quorum membership verification**

This repository develops a decentralized storage network for PS5, Windows, Linux, and macOS. The desktop code implements a **LOCAL**, AES-256-GCM-encrypted, content-addressed vault with durable 90-day retention; separately tested Reed-Solomon recovery and cryptographically validated short-lived membership tickets have now been added, but are not yet connected to a P2P service. **It is not yet a working P2P network or native PS5 application.**

## Non-negotiable network membership rule

**Every participant contributes at least 10 GiB of verified, usable storage. No guest mode.** Without a cryptographically authenticated identity and a current, independently audited 10-GiB storage proof, the future network must deny ALL operations: discovery, search, listings, CID resolution, metadata, uploads, downloads, publication, replication, and relaying.

`src/core.cpp` implements the shared fail-closed admission policy. `preflight()` requires 10 GiB configured and the offered quota **plus 2 GiB emergency headroom** initially free locally. Free-space checks are **not** remote proof-of-storage; neither UI settings nor sparse files count. Until independently audited peer proofs, quorum issuer nodes and authenticated relay transport are available, the entire network remains disabled. The new verifier can check at least 3 trusted Ed25519 signatures plus a fresh channel-bound node challenge, but it cannot create actual storage audits.

**Local import/export commands are offline file-management operations on the user's own vault**, not uploads/downloads from the P2P network. They never make the owner a member.

## 90-day file expiration

- A file expires **90 x 24 hours after initial successful import** unless completely restored/downloaded.
- A **fully authenticated, integrity-verified, successfully finalized** local restore renews the expiry to **90 x 24 hours after that restore**.
- Failed/wrong-key/partial restores, CID lookups, file searches, heartbeats, and metadata reads **do not renew** the timer.
- On any local vault command, the app automatically runs expiry collection. `--prune` runs it manually.
- Expired records are removed in a SQLite transaction; physical encrypted blocks are deleted **only if no live file references them**. Downloaded copies outside the vault are never touched.
- TTL is stored as integer UTC epoch seconds, not local timezone dates. The system clock must be trustworthy; future network use requires authenticated completion receipts and clock-skew policy.

**Replication across peers, distributed expiration, anti-replay download receipts and a network GC consensus are not implemented yet.**

## Large files, no artificial per-file limit

Files are streamed in **4 MiB encrypted chunks** with constant-size data buffers; the CID file utility streams with a 1 MiB buffer. Files are not read entirely into RAM. The previous 32 MiB CID command limit is removed.

The remaining limits are the underlying filesystem, free disk space, available block index memory, and the metadata/database's signed 64-bit file-size representation (up to approximately 8 EiB). This is not an unlimited quota: nodes still have an allocation and must preserve disk headroom.

Each local file gets a **CIDv1** manifest. The canonical on-disk manifest contains the protocol version, a random 128-bit per-file salt, the exact file length, and ordered SHA2-256 CIDs of ciphertext chunks. Chunk and manifest integrity is checked on restore. Block encryption uses **AES-256-GCM** with random 96-bit nonces, with chunk index and file salt bound as authenticated data. A fresh random 256-bit file key is written to a separate user-designated key file. **Do not lose the key!** Keys are not published in CIDs.

For comparison, `--cid-file FILE` computes a CIDv1 of the **raw file bytes**; that differs from the manifest CID of the encrypted imported object. A shareable CID is an identifier, not a decryption key or a membership bypass.

## Desktop build and tests

C++20, CMake, OpenSSL 3-compatible Crypto API, SQLite3.

```sh
cmake -S homebrew/ps5-meshcloud -B build/meshcloud -DCMAKE_BUILD_TYPE=Release
cmake --build build/meshcloud --config Release
ctest --test-dir build/meshcloud -C Release --output-on-failure
```

GitHub Actions: [Build MeshCloud Desktop](../../.github/workflows/build-meshcloud-desktop.yml) targets Ubuntu x64, Windows x64, and macOS arm64. CI tests membership denial, standard CID vector, streaming 35 MiB roundtrip, wrong-key recovery rejection, exclusive vault process locking, time-driven expiry + physical block deletion, Reed-Solomon recovery with 20 or 100 missing shards, 3-of-5 independent auditor signatures and bounded authenticated wire frames. A workflow definition in GitHub does **not** establish successful CI execution; check the Actions run result.

### Desktop CLI examples

```sh
meshcloud --help
meshcloud --storage ./mesh-vault --quota-gib 10
meshcloud --cid-file /path/to/large-file.iso

# Local-only encrypted file import / export (NOT an upload/download to other nodes):
meshcloud --storage ./mesh-vault --local-import /path/to/large-file.iso --key-out ./my-private.key
meshcloud --storage ./mesh-vault --info bafkrei...
meshcloud --storage ./mesh-vault --local-export bafkrei... --key-file ./my-private.key --out ./recovered.iso
meshcloud --storage ./mesh-vault --prune
```

The import prints a full manifest CID to use with local info/export. An existing output file or key file will not be overwritten. Keep the private key file on a trusted disk, ideally outside the shared storage folder. Disk storage used by incomplete or interrupted imports can leave orphan blocks that are collected by a subsequent prune.

## Security boundaries and work required before a real release

- **Unimplemented:** actual P2P encrypted transport/listener, DHT discovery, relay-only mode, public search indexing, interoperable network CID fetching, shard transfer, actual disk-backed audit issuing, codec-to-vault integration, fault-domain placement and repair, native PS5 UI/package. Reed-Solomon coding and quorum-signed ticket **verification** are implemented and tested as separate components.
- **Unimplemented:** parallel writers within one vault (an OS-level exclusive lock instead rejects concurrent vault processes), operational DoS/rate-limit controls, anti-Sybil enforcement, an encrypted private catalog, crash-safe fsync guarantees on every platform, independent security review.
- **Security limitations:** Key-file ACLs rely on host OS filesystem permissions; isolate the vault from untrusted local users. A local CLI cannot prove 10 GiB storage to remote nodes. Nonces are random and AES-GCM must never be reused with the same key/nonce pair.
- **IP privacy:** Even relay-only networking would reveal a participant's connection IP to the relays, which can also observe Peer IDs. Direct P2P exposes IPs to other direct peers. There is no guarantee of anonymity. See [libp2p Circuit Relay](https://libp2p.io/docs/circuit-relay/).
- **Legal operations:** Future distributed content sharing needs abuse reporting, opt-in sharing, user data protection and legitimate takedown procedures.

These limitations are intentional: do not run an unaudited storage service on the open Internet merely because an offline prototype compiles.

## References

- [libp2p documentation](https://docs.libp2p.io/)
- [Tahoe-LAFS concepts](https://tahoe-lafs.readthedocs.io/)
- [PS5-DevBridge](https://github.com/Yahballon/PS5-DevBridge)

## New in v0.3.0: offline-tested network security building blocks

- `src/erasure.cpp`: systematic Reed-Solomon GF(256) `k+m` fragment encoding and recovery, including **10+20** or **10+100**. With verified CIDs, any `k` remaining distinct fragments can reconstruct a stripe. The 10+100 setup costs 11x storage and needs 110 independently placed fragments to withstand 100 lost nodes.
- `src/admission.cpp`: require 3 distinct trusted Ed25519 auditor signatures on a <=1-hour 10-GiB storage ticket; confirm peer-key control with a fresh session challenge bound to an authenticated channel exporter. **There are no operational auditor nodes or ability to self-issue production membership.**
- `src/wire.cpp`: strict bounded (4 MiB + 4 KiB) framed network message codec. Parsed search, CID metadata, upload and download messages fail closed without verified membership. **This is not a secure socket server**.
- See [ARCHITECTURE.md](./ARCHITECTURE.md) for precise limitations, placement guarantees, planned relays and privacy threat model.

No guest access or network bypass was introduced. The v0.3.0 desktop pre-alpha remains a local vault/diagnostic client rather than a deployed cloud network.
