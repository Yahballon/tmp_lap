# MeshCloud PS5 Native v0.4.0

**By YAHBALLON** — native controller-first OpenGL app with a directly connected, member-authenticated TLS1.3 LAN peer engine.

Title ID: PPSA99718
Content ID: UP9000-PPSA99718_00-MESHCLOUDPS5APP1
Build workflow: .github/workflows/build-ps5-meshcloud.yml

## Implemented

The native PS5 app has its own 1920×1080 OpenGL interface using the PS5-DevBridge homebrew UI. It displays five controller-navigable pages: Overview, Storage, Local files, Network and About.

Storage checks use POSIX statvfs on /data, /mnt/usb0 and /mnt/usb1 when available. They display **filesystem-reported** free bytes and whether at least **13 GiB + 2 GiB safety margin** appears free. This is not actual storage reservation, proof of storage, validation of a contribution, or permission to use the P2P network.

The Local files page lists regular files from <selected mount>/PS5MeshCloud/files without following symbolic links. It does not scan or open private keys, privileged console files or copyrighted content outside those paths. The read-only listing itself does not publish data; the Network page can explicitly PUT a **selected encrypted MCB1 fragment** or GET a CID from an independently audited, pinned peer.

A best-effort log is written to /data/PS5MeshCloud/meshcloud-ps5.log when writable. A direct TLS1.3 peer listener is started **only after the user presses X** on the Network page. Without a valid independently signed membership ticket, the connection fails closed.

## Controller

- D-pad left/right or L1/R1: move among pages.
- D-pad up/down: move among pages; scroll local files when on Local files.
- Triangle: on Network, GET the configured CID; elsewhere, refresh local mount/file observations.
- Square: on Network, PUT a selected MCB1 encrypted fragment; elsewhere, switch selected storage mount (/data, /mnt/usb0, /mnt/usb1).
- Cross: on Network, listen for one authenticated P2P request; elsewhere, select or inspect.
- Options: About.
- Circle: go to Overview, then close the app.

## Build and installation

GitHub Actions checks host-side filesystem diagnostics, compiles the application using PS5-DevBridge, generates an original blue MeshCloud app icon, verifies installer metadata/PS5 executable signature and builds PPSA99718.zip.

When GitHub Actions has **actually passed**, extract PPSA99718/ into /data/homebrew/ and register it with a supported PS5 homebrew loader such as ShadowMountPlus. Compilation is not a substitute for hardware tests; the application should still be tested on a jailbroken PS5.

## Strict limitations

**Only direct audited TLS 1.3 encrypted-fragment exchange is supported at this stage**. The native executable now links the shared C++ CID, AES-GCM vault, SQLite, Ed25519 membership and exchange components, but the PS5 UI presently exposes limited fragment PUT/GET rather than a complete whole-file cloud. Real 13-GiB auditor servers, encrypted file publication/indexing, relay transport and global file retention are not available yet.

In particular:
- No access to searches, CIDs, metadata, uploads or downloads for unverified members.
- The 13-GiB rule applies to ALL members and platforms; free-space results alone do not pass the proof. A valid 3-of-5 externally issued proof ticket must be supplied.
- A signed 90-day file expiry is planned at network level. The PS5 prototype cannot yet enforce it against remote peers.
- Network IP privacy is not implemented. A real relay would still see its client IP.
- The custom app icon has no Sony or PlayStation trademark graphics.

Keep sensitive files out of the files folders until production encryption and policy handling exist.

## Direct P2P LAN transport v0.7 / PS5 native v0.2

MeshCloud now includes `src/p2p_tls.cpp`: a **symmetric TCP peer** implementation. Both nodes can listen for connections and initiate them. It uses pinned Ed25519 mutual TLS 1.3 and a fresh TLS-exporter-bound proof of a short-lived, three-auditor-signed ticket for **at least 13 GiB**. Nobody receives MeshCloud CIDs, shards, listings, or searches before both TLS certificate pinning and member verification succeed. The core's GET/PUT frames carry encrypted `.mcb` chunks, never plaintext or file keys.

Windows/Linux/macOS gain `meshcloud-peer` in addition to the existing CLI and the Windows GUI. The native PS5 app has a Network page to listen, PUT a selected local MCB1 ciphertext block and GET a configured CID after cryptographic admission. The **same** signed ticket binary, trusted auditor keys and allowlist format are used on every system.

For a deliberately provisioned two-device LAN:

1. Arrange **genuine, externally audited, quorum-signed 13-GiB member tickets** for both peers. There is no built-in credential minting, shared test password, guest mode, or self-attestation.
2. Put files `identity.pem` (private Ed25519 PEM key), `member.ticket` (canonical signed binary), `auditors.keys` (minimum three independent pinned auditor Ed25519 public keys, 64 hex characters per line) and `peers.keys` (the exact other node's public key in hex) in each node's private config directory. Never publish private keys or reuse test signatures as genuine audits.
3. On the PS5 use `/data/PS5MeshCloud/p2p/` and a `network.conf` containing `bind=192.168.x.x`, `peer=192.168.x.y` and optionally `cid=bafk...`. Replace IPs with the actual PS5 and PC addresses.
4. On PC: `meshcloud-peer listen ./pc-config ./pc-shards 192.168.x.y`. Use the PC's real LAN IP, not `0.0.0.0`. One listener handles one authenticated request.
5. On the PS5 Network page press **Square** to PUT the selected encrypted `.mcb` fragment from the Local files page, or **Triangle** to request the configured CID from the PC.
6. In the opposite direction press **X** on PS5 Network to listen, then use `meshcloud-peer put ./pc-config 192.168.x.x encrypted.mcb` or `meshcloud-peer get ./pc-config 192.168.x.x CID recovered.mcb`.

The node must ALSO pass the local **13 GiB + 2 GiB headroom** preflight; this disk-space check is not a replacement for the remote proof. Shard capacity remains capped at 13 GiB per node in this prototype. A `.mcb` is an encrypted 4-MiB-or-smaller fragment, **not a whole-file automatic upload**. The original whole-file local vault still supports streaming large files.

**Important status:** The repository still needs a production-grade proof-of-storage issuer/auditor network. Without valid independent audit credentials, the real P2P connection **correctly refuses access**. A passing CI test uses ephemeral mock auditors and does not create valid production tickets. Direct IP on LAN exposes peer LAN addresses; relay-only internet mode, NAT traversal, DHT and distributed 90-day file retention are not yet complete. Treat this as a controlled direct-LAN engineering test only.


## Bitcoin-inspired v0.8 peer discovery

Nodes now maintain a persistent signed SQLite address book and exchange bounded signed LAN endpoints **only** inside fully authenticated TLS 1.3 / 13 GiB membership channels. Peers can reconnect after restart using cached endpoints rather than a master server. Explicit local-IP advertising is optional. The PS5 Network page has R2 for signed GETADDR. See [P2P_DISCOVERY.md](../P2P_DISCOVERY.md) (or the repository root P2P_DISCOVERY.md) for bootstrap details and current security limits.

## v0.10 - 13 GiB + real independent audit-only TLS port

Per-node contributed storage is **exactly 13 GiB (hard cap)** with a separate 2 GiB preflight reserve. New peers claiming less OR more are denied; three independent signed proofs remain mandatory. The new `meshcloud-audit-net` runs mutual Ed25519 pinned TLS 1.3 on TCP 39719 and verifies live 48-block random storage challenges before issuance. It is NOT a CID or guest file endpoint. PS5 Network L2 starts one separate pinned audit listener. See [STORAGE_AUDIT.md](./STORAGE_AUDIT.md) for honest limits: 13-GiB audit dataset must currently be provisioned out of band; live 3-auditor ticket aggregation and unattended onboarding are still unavailable.
