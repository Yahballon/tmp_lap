# MeshCloud by YAHBALLON

**v0.6.0 pre-alpha – encrypted local vault, CIDv1, Reed-Solomon redundancy and quorum membership verification**

This repository develops a decentralized storage network for PS5, Windows, Linux, and macOS. The desktop code implements a **LOCAL**, AES-256-GCM-encrypted, content-addressed vault with durable 90-day retention. A separate v0.5 module simulates two member-authenticated nodes exchanging ciphertext blocks by CID, backed by SQLite and strict quota limits; no Internet or LAN listener is enabled. **It is not yet a working P2P network or native PS5 application.**

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

## Windows native GUI (v0.4.0)

The **Windows x64** archive now contains **two separate executables**:

- `MeshCloud-GUI.exe` — real native Win32 interface (no browser/server/Electron).
- `meshcloud.exe` — original command-line program for scripting and diagnostics.

Both share **the exact same encrypted C++ storage engine and SQLite vault**.

The GUI includes: editable vault folder and folder picker, local encrypted file list (name, size, expiry, CID), native file dialogs for imports and restores, generated private-key export, CID copy, local raw-file CID hashing, 90-day expiration cleanup, progress indication, cancellation of chunk-based imports/restores, and an always-visible **NETWORK LOCKED** state.

Operations run in a background worker; the UI remains responsive while handling large files. The GUI refuses to overwrite an existing key or recovered file. Select a file in the table to populate its CID, click **Restore selected**, choose its matching saved `.key` file, then choose an output path.

The GUI does **not** falsely pretend to be connected to other PS5 or desktop nodes. It cannot currently search the public MeshCloud network or perform a remote upload/download. The 10-GiB contribution rule remains mandatory for all future network features; local offline imports/restores do not grant network membership. The GUI is a pre-alpha file manager, not a released P2P cloud client.

**Keep your private `.key` files outside the vault**, backed up somewhere secure. They are not embedded into CID links. Moving or deleting the key can make the stored data unrecoverable.

Existing v0.3 SQLite databases are upgraded in place to show original filenames. The migration does not reset CID, retention timestamps, or encrypted data; nevertheless back up important data before running a pre-alpha application.

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

No guest access or network bypass was introduced. The v0.4.0 desktop pre-alpha remains a local vault/diagnostic client rather than a deployed cloud network.

## v0.4 desktop build changes

GitHub Actions builds both Windows apps in the same MSVC x64 job. The artifact `meshcloud-windows-x64.zip` contains `MeshCloud-GUI.exe` **and** `meshcloud.exe`, plus the OpenSSL/SQLite DLLs required by the dynamic vcpkg toolchain. Linux and macOS retain the CLI. The GUI backend adds migration, file listing, cancellation and stream roundtrip tests to CI.

The Windows GUI is an offline app in this pre-alpha. It does not collect or transmit IP addresses because it opens no P2P connections. Once real network connections exist, relays will still observe the client's IP; anonymization is not guaranteed.

## v0.5 – Verified CID shard exchange (developer-only, no open sockets)

The cross-platform core includes a member-only request handler for **encrypted** chunk upload/download by CID. Its tests create two simulated nodes and ephemeral quorum-signed membership credentials, then verify that:

- Unverified clients cannot upload or download, even if they know the CID.
- Chunks are addressed by the **hash of their ciphertext**, and changed bytes are rejected on upload and again on download.
- Only canonical CIDv1 is accepted, with strict 4 MiB chunk limits per frame, while complete files remain streamable without a user-facing size cap.
- Encrypted chunks reside in a separate shared-fragment store, not the private local vault, with SQLite quota metadata and an independent 90-day staging expiry.
- A successful fragment GET cannot extend the entire file's inactivity period, and a malformed or expired session gets no response.

This is **not yet a functioning WAN/LAN cloud**: peer transport over relays, real 10 GiB storage audits, full-file receipt proofs, signed manifests, DHT filename search, multi-node placement, automatic shard repair and native PS5 installation remain unfinished. Importantly, the current Windows GUI **still operates locally**; it does not expose any P2P ports.

See [ARCHITECTURE.md](./ARCHITECTURE.md) for the threat model, the difference between per-shard staging TTL and the user-requested 90-day full-file lifetime, and planned independent relays.

## v0.6 – Publisher-signed, paged ciphertext CID manifests

`src/published.{hpp,cpp}` signs immutable **ciphertext-block CID pages** with Ed25519. Each page contains up to **128 encrypted block CIDv1 references**, their exact ciphertext byte lengths, the canonical parent file-manifest CID, a publisher public key, ordered page position and count, original publication time, signed lease issue time, and 90-day expiry. Pages use deterministic versioned binary transcripts with domain separation. No plaintext, filename, AES key or private identity key is exposed.

The verifier requires the **expected publisher public key**, a valid Ed25519 signature on every page, all page indices in order (no gaps/repeats), identical parent CID and publication metadata, and an overflow-safe exact byte sum. Altered/missing pages, a wrong publisher, expired signatures and malformed CIDs are rejected. A 90-day lease renewal requires **fresh publisher signatures**; a search or partial block GET alone cannot extend it. Arbitrarily large everyday files can be represented with additional pages rather than a fixed 32-MiB file limit, subject to host 64-bit storage limits.

`tests/test_published.cpp` includes a mock three-auditor membership quorum, two separate SQLite stores, publisher signatures, a signed multi-page CID index and re-verified ciphertext chunk GETs. The mock ciphertext envelopes do not demonstrate real AES-GCM decryption.

**Still missing for a public P2P launch:** secure sockets/relays and channel binding, production anti-Sybil storage auditors for the 10 GiB rule, signed manifest distribution/search, erasure-coded placement/repair, cross-node verified complete-file download receipts and native PS5 packaging. Existing GUI operations remain offline.
