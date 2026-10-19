# MeshCloud PS5 v0.2.0 pre-alpha – direct audited TLS1.3 P2P

Native PS5 OpenGL UI, mounted storage diagnostics, Ed25519 pinned TLS 1.3 peer link and encrypted CID chunk GET/PUT.

**A real PS5↔PC peer connection requires externally issued 3-of-5 signed storage audit tickets proving at least 10 GiB each.** No ticket or non-pinned peer: no CID lookup, metadata, upload or download. The app includes NO issuer, no self-signed membership bypass and no production auditor network. This is an audited-LAN transport prototype, not the finished public cloud.

PS5 configuration: /data/PS5MeshCloud/p2p/identity.pem, member.ticket, auditors.keys, peers.keys and network.conf. Encrypted MCB1 blocks can be placed into /data/PS5MeshCloud/files. The native Network tab starts a one-request listener with X, PUTs a selected encrypted block with Square, or GETs the configured CID with Triangle. Direct port 39718, one operation per listener. No automatic discovery or relay, so direct peers see each other's LAN IP.

Source and CI: https://github.com/Yahballon/tmp_lap/tree/main/homebrew/ps5-meshcloud

Native PS5 hardware validation remains required after a successful GitHub build. Build result is not proof of secure peer storage auditing.
