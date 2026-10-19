# MeshCloud v0.8 – Bitcoin-style decentralized discovery

This reuses the Bitcoin addrman and getaddr idea, not its blockchain or mining. No central master is needed after one known initial peer.

## Implemented on PC and native PS5

- Mutual TLS 1.3, Ed25519 pinned identities, current 3-auditor signed 10 GiB storage membership remains mandatory before GETADDR or file operations.
- Self-signed opt-in private LAN IPv4 announcements: port, signed time, 7-day expiry.
- Bounded member-only getaddr: max 16 records, each 122 bytes; a requester cannot announce another person's endpoint.
- Persistent signed peer cache in addrman/addrman.sqlite with at most 512 identities, stale entry cleanup, replay-resistant timestamp updates.
- Nodes learn peers from an authenticated member and can dial cached peer addresses across restarts. Manual bootstrap to one existing peer is still required initially.
- No hardcoded master or default auditor keys. No DNS request, public IP gossip or automatic address sharing.

## PC terminal

All commands require identity.pem, member.ticket, auditors.keys and peers.keys in the config directory. Certificates are pinned, tickets require externally verified at least 10 GiB and are NOT minted by this tool.

```powershell
meshcloud-peer.exe listen-loop C:\MeshCloud\config C:\MeshCloud\shared 192.168.1.20
meshcloud-peer.exe discover C:\MeshCloud\config 192.168.1.50
meshcloud-peer.exe peers C:\MeshCloud\config
meshcloud-peer.exe discover-auto C:\MeshCloud\config
```

Put just your own private RFC1918 IPv4 in config/advertise.ipv4 to OPT IN to publishing your own endpoint. If this file is absent, the node may still request other addresses but does not advertise its own IP.

## PS5

Set bind=192.168.1.50 and peer=192.168.1.20 in /data/PS5MeshCloud/p2p/network.conf. Optional: advertise=192.168.1.50.
On Network press X to accept one member-authenticated request and R2 to GETADDR from PC. Square sends a selected encrypted chunk and Triangle fetches a configured encrypted CID. Cached signed peers live in /data/PS5MeshCloud/p2p/addrman/addrman.sqlite.

## Hard limitations

No production independent proof-of-storage auditor network yet issues valid 10 GiB tickets, so a public stranger cannot join. CI ephemeral test tickets demonstrate protocol checks but are NOT production credentials.
No global DHT, public relay mesh, NAT traversal, IPv6, automated replication, automatic continuous peer maintenance or general file-name search is claimed. Current address gossip ONLY supports local private IPv4, preventing private LAN IP leakage onto future public links.
Bitcoin-like address gossip alone does not prevent Sybil/eclipsing; robust diversity, ongoing reachable-peer checks and rate limiting remain necessary before a global launch.
