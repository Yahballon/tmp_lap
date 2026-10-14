# Deep Signal design research

Research checked on 2026-10-14. The references are examples of pacing and
readability, not source maps or artwork used in this game.

Primary sources inspected:

- [Nintendo: Super Mario Bros. NES instruction booklet](https://www.nintendo.co.jp/clv/manuals/en/pdf/CLV-P-NAAAE.pdf), printed pages 5-8: side-view terrain, finish-area screenshots, held jump height and powerup affordances.
- [Nintendo: Super Mario Bros. 2 NES instruction booklet](https://www.nintendo.co.jp/clv/manuals/en/pdf/CLV-P-NAADE.pdf), printed pages 6-8 and 15: area/boss progression, height and movement screenshots, interaction examples.
- [Nintendo: Super Mario Bros. 3 NES instruction booklet](https://www.nintendo.co.jp/clv/manuals/en/pdf/CLV-P-NAACE.pdf), printed pages 9-12: world-map presentation, flight/vertical options and contrasting movement interactions.
- [Nintendo: Designing Levels Together](https://www.nintendo.com/en-gb/Iwata-Asks/Super-Mario-Bros-25th-Anniversary/Vol-5-Original-Super-Mario-Developers/4-Designing-Levels-Together/4-Designing-Levels-Together-212908.html): anticipating player actions when placing terrain and enemies.
- [Nintendo: Applying a Single Idea to Both Land and Sky](https://www.nintendo.com/en-gb/Iwata-Asks/Iwata-Asks-New-Super-Mario-Bros-Wii/Volume-2/5-Applying-A-Single-Idea-To-Both-Land-And-Sky/5-Applying-A-Single-Idea-To-Both-Land-And-Sky-233080.html): communicating viable routes through coin placement.

Our design conclusions and implementation:

| Principle inferred from references | Original Starbound implementation |
| --- | --- |
| Introduce an interaction before combining dangers | Entry supplies and isolated stair/fissure/swim bays before mixed encounters |
| Make jumping and movement routes readable | Coins above safe jumps, broad treads, alternating coral columns, visible airlock exits |
| Offer a distinct identity per area | Surface expedition, crystalline quarry, cargo sky bridges, four submerged observatories and reactor chambers |
| Make optional routes useful | Upper shelves, hidden reserve cells, cargo lifts and elevated maintenance routes |
| Let the player recover before a new challenge | Cleared service pads, checkpoint retries, enemy-free spawn buffers |
| Finish a major area with a clear objective | Reactor sentinel plus cut-off terminal, bridge shutdown and campaign finale |

The game retains its own physics and controls. It does not implement Mario 2's
enemy carrying, Mario 3's raccoon flight, slope physics, copied stages or Nintendo
enemy identities. Instead, the ideas above are adapted to the actual ECS and
platform constraints. Static geometry checks exclude enemy AI and timing and
must be followed by PS5 playtests.
