# Open Surge PS5 v0.5.3 — Original layer and tube routing

Fixes two missing features identified by the user's console report: original per-brick green/yellow collision layers were silently discarded, and all 12 original Tube In/Out entities were ignored.

- Retain and filter the real 11 green/yellow bricks per `obstaclemap.c` default/green/yellow rules.
- Read original Layer Green / Layer Yellow 32x32 trigger positions.
- Basic automatic native Tube In/Out transport at 600 units/sec for matching authored routes (controls frozen inside); bidirectional endpoint tests included.
- Native control/audio and Spin Dash uphill regression preserved.
- **No complete 360-degree loops yet.** Original `physicsactor.c` rotates six sensors into four movement modes and also handles layers/tubes through SurgeScript. This native preview still lacks that complete engine.
- On-console feedback requested. Do not describe full tube topology or loops as done.
