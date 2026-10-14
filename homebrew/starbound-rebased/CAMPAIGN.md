# Relayfall campaign

An original 32-stage expedition to restore an abandoned planetary relay network.
The existing HD astronaut, sci-fi atlases, input mapping and player transforms
are preserved. Maps do not use upstream level geometry.

| Sector | Stages | Gameplay progression |
| --- | --- | --- |
| Landing Reach | Arrival, Supply Trail, Survey Ridge, First Relay | Energy crates, held jumps, safe stairs, first slime patrols |
| Crystal Quarry | Fault Line, Crystal Steps, Cache Run, Quarry Relay | Short fissures, broad elevated treads, optional crystal caches |
| Spore Gardens | Green Frontier, Crawler Beds, Canopy Cache, Garden Relay | Slimes and tall crawlers, raised escape perches |
| Cargo Ring | Dock Approach, Lift Yard, Upper Cargo, Freight Relay | Optional vertical lifts and upper cargo rewards |
| Dark Observatory | Night Watch, Armored Gallery, Signal Cache, Lens Relay | Armored patrols, optional invulnerability cells |
| Ion Foundry | Cooling Trench, Machine Floor, Maintenance Route, Forge Relay | Alternating combat, lifts and three-tile fissures |
| Storm Array | Broken Links, High Voltage, Hidden Frequency, Storm Relay | Four-tile fissures and combined traversal encounters |
| Relay Core | Core Approach, Security Grid, Reserve Cells, Signal Restored | Combined skills, reserve life route and campaign finale |

Every stage has an entry supply crate before danger, six intentionally ordered
encounter bays, a cleared service checkpoint at tile (98,12), a second supply
crate, and a relay exit with the solid wall required by FlagSystem's auto-walk.
Powerups are helpful but never required to traverse the main route. Growth
crates use the existing mushroom/fire upgrade logic. Armor bays offer a star
before the patrol. Reward shelves contain coins and an invisible reserve-life
block. These secrets reward exploration; they do not skip stages or trap players.
Lifts offer optional upper rewards above solid floors. There are no forced waits,
mandatory breakable barriers, blind warp destinations or enemy-gated exits.

Checkpoints activate only when the living player stands inside the marked pad.
Death still costs one life and restores the level's enemies/items, but respawns
at the service pad with the camera moved there. A level transition resets the
checkpoint. Checkpoints are session-only; quitting or game over clears them.
All 8x4 stages can be selected in the existing menu for TESTBUILD evaluation.
Stages link sequentially across sectors; 8-4 ends at (0,0), plays victory music
and shows a completion prompt returning to the menu on confirm.

## Engine contract

Six rectangular 15-row CSV layers are loaded in lockstep; -1 is empty.
Foreground IDs: 384/385 terrain, 197 solid panels, 192 animated energy crates,
144 coins, 149 relay pole, 152 descending beacon, 609 vertical lift.
Collectibles: 608 growth, 96 invulnerability, 52 reserve life. These are dispenser
metadata at crate cells, or an invisible block for the hidden reserve life;
loose coins belong in Foreground, not Collectibles. Enemy IDs: 70 slime,
38 tall crawler, 87 armored crawler. Tall enemies start one row higher.
All IDs retain the native atlas geometry and engine semantic lookup.
`CAMERA_MAX` is the absolute right boundary in tiles, not the maximum scroll
offset. Moving-platform properties use the engine's coordinate/enumeration
syntax. Empty optional arrays are separated by blank lines to terminate the
existing regex parser. `CHECKPOINT` extends the level parser with one coordinate.

## Authoring and validation

The tracked `campaign/` directory contains all 192 CSV maps, 32 property files
and a campaign manifest. `tools/build-campaign.py` reproduces them from the
explicit sector/encounter plan without RNG or imported map data. Change that
plan and regenerate rather than hand-editing generated maps.

```sh
python3 homebrew/starbound-rebased/tools/build-campaign.py
python3 homebrew/starbound-rebased/tools/check-campaign.py
```

CI checks all layer sizes, supported semantic IDs, unique geometry, safe spawns,
enemy support, checkpoint clearance, platform property matching, complete exit
assemblies and the exact sequential transition chain including the finale.
It also checks static route reachability for both 32px and 64px player heights
using pinned engine jump/gravity constants. This simplified model excludes enemy
AI, dynamic lift timing and destructible shortcuts; it is not a complete engine
or controller playtest. Negative tests corrupt exits, IDs, checkpoints and
transitions to ensure the validator fails. CI installs the maps after all raster
generators/reference work, then checks packaged bytes against the tracked maps.

## Hardware acceptance checklist

Install the v0.1.20 TESTBUILD (PPSA99642) with rollback available. Test menu
selection for 1-1, 4-2, 8-4; sequential 1-4 to 2-1; death before/after a checkpoint;
both player sizes across steps and fissures; growth then fire upgrade; reserve
life discovery; vertical lift boarding; all patrol families; the final relay and
confirmation back to the menu. Confirm the v0.1.19 walking/mushroom fixes remain
correct. Native CI compilation and static checks cannot replace these PS5 tests.
