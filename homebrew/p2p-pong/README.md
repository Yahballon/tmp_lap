# P2P PONG v0.1.0 by YAHBALLON

Native PS5 multiplayer Pong proof of concept with PC-hosted open-source rendezvous/relay.

## Features
- Choose player name and lobby name using the DualSense
- Create a room, find public rooms, refresh, join with X and start with host
- 2-player authoritative Pong simulation: host simulates physics, guest sends input, host shares state
- Both consoles connect *outbound* to a publicly reachable relay. **No port forwarding on either player's home router.**
- No central account, proprietary master service, or game logic on the relay.
- Run your own relay; this first version is **not** decentralized global peer discovery, true direct P2P, encryption or anti-cheat. One connected relay is necessary. Later versions can add DHT, QUIC, NAT traversal, signed identities, peer relays and multi-relay gossip.
- Logs: `/data/P2PPong/logs/pong.log`.

## Setup
1. Run the relay on an Internet-reachable Linux PC/VPS: `python3 relay/relay.py`. The relay listens on TCP port 39733. Public firewall must allow this port.
2. On each PS5 put in `/data/P2PPong/network.conf`:
   ```ini
   relay=YOUR_PUBLIC_RELAY_IP
   port=39733
   ```
3. On both consoles: launch P2P PONG, choose a player name, create or find a lobby.
4. Host presses X after guest joins. Move paddles with D-Pad Up/Down, Circle exits.

**Security notice:** v0.1 is a lab prototype; relay traffic is unencrypted and relay can read/modify match state. Do not expose this experimental public service to strangers until transport authentication, encryption, rate limits and abuse protections are added.

## Planned
M2: tested NAT hole punching/relay fallback and multiple independent bootstrap nodes.
M3: cryptographically signed identities, encrypted messages, reconnects, host migration.
M4: cross-platform PC Pong client, lobby filtering and automatic relay discovery.

See [MILESTONES.md](MILESTONES.md).
