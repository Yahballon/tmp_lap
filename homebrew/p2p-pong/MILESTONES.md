# P2P PONG — Agent handoff milestones (English)
- M0 (2026-10-23): Repository scaffold, game physics and TCP rendezvous relay protocol implemented.
- M1 (2026-10-23): Native PS5 UI implemented for player name, lobby name, room search/create/join/start and 2-player synchronized Pong. **Needs GitHub CI build and actual console test.** Router forwarding is not needed at home when clients dial a public relay; public relay infrastructure is required.
- M2 TODO: Add authenticated encrypted transport (Noise/TLS), connection resilience, replay protection and robust framing. v0.1 relay must be treated as an untrusted experiment.
- M3 TODO: Add decentralized peer list and lobby propagation between independent relays; ICE-like hole punching (including symmetric NAT fallback). Bootstrap is unavoidable for initial network entry.
- M4 TODO: Windows/Linux/macOS desktop game clients, PS5-PC crossplay, latency/packet loss telemetry.
- M5 TODO: TESTBUILD release with verified PS5 hardware video and logs, screenshot and HQ icon.

## Technical notes
- Shared gameplay physics: core/pong.hpp.
- Relay: relay/relay.py, newline ASCII protocol. NAME, CREATE, LIST, JOIN, START, INPUT, STATE.
- PS5 frontend: native/main.cpp; TCP transport: native/network.hpp.
- PS5 Title ID PPSA99873 (verify noncollision before release).
- Do NOT borrow MeshCloud storage membership or impose the 13-GiB storage requirement on gameplay. Reuse conceptual discovery patterns only.
- Next agent should fix any platform compile errors and validate on real PS5 before tagging production.
