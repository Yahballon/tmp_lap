# Relayfall: Moving Frontiers campaign

32 original stages across eight sci-fi sectors. Deep Signal replaces the repeated
v0.1.20 half-level templates with six individually ordered encounter bays and
larger maps (224–374 tiles). Player controls, jump physics and the repaired
walking/growth atlas remain compatible.

| Sector | Gameplay identity |
| --- | --- |
| Landing Reach | Supplies, stairs, fissures, island valleys and early patrols |
| Crystal Quarry | Crystalline terrain, low tunnels and layered reward shelves |
| Spore Gardens | Crawlers, patrol perches and alternating tunnel/valley paths |
| Cargo Ring | Static sky bridges, optional lifts and new air sentries |
| Tidal Observatory | Four genuine underwater stages: swim training, coral channels, jelly pursuit and deep caches |
| Ion Foundry | Industrial traversal followed by the 6-4 reactor sentinel |
| Storm Array | Combined drone, sky-bridge, armor and fissure challenges |
| Relay Core | Mixed-skill stages and the final 8-4 sentinel shutdown |

Every stage has entry supplies, a service checkpoint at (98,12), a second supply
crate and a readable exit. Checkpoints activate only for a living player standing
on the marked pad; life costs and level resets remain. Enemy spawns are cleared
around the pad. Session checkpoints reset when changing stages or leaving the game.
All 8x4 stages are selectable for TESTBUILD evaluation and link in order.

Optional reward shelves/lifts contain coins and hidden reserve cells. Growth
crates use the existing mushroom/fire upgrade logic. Later armor bays offer a
temporary invulnerability cell. Main dry routes are statically traversable with
either player size without mandatory powerups. All 28 dry stages now require
boarding a lift and jumping onto a seven-to-nine-tile-high ledge. Twelve cargo/storm/core
stages also contain a 28-tile ferry crossing; missing a ride never strands the ferry
off-camera. There are 54 actual platforms, using three-tile 809 deck entities. Swim stages use
repeated jump presses, with three-or-more-tile corridors for the large player.

Boss arenas use a connected reactor-sentinel sprite, bounded patrol/jumps and
projectile attacks. A visible elevated maintenance route leads over the encounter
to the cut-off terminal. Activating it collapses the bridge beneath the sentinel,
then advances to the next sector. The final arena shows the signal-restored prompt
and returns to the menu on confirmation. Boss shutdown does not require fireballs;
the upstream fireball HP branch is not implemented and is not claimed here.

## Engine and map contract

Six rectangular 15-row CSV layers load together; -1 is empty. Ground aliases
384/385, 400/401, 416/417 and 912/913 share terrain semantics while carrying four
original palettes. Solid panel aliases are 197,213,229,725. Energy crates use192;
loose coins use Foreground144. Collectibles608/96/52 specify growth, star or reserve
life at crate cells; reserve-life metadata on an empty cell becomes a hidden block.
Enemy IDs38/70/87 are existing crawlers/slimes; new campaign types81/48/90/61 are
ray/jelly/sentry/sentinel. Tall enemies spawn one row higher; air/water enemies do
not need floor support. Real platforms 809 have matching vertical/horizontal movement properties.
The previous609 map tile was a static block in the upstream factory and is
no longer used in this campaign; the factory is repaired for future one-tile lifts.

Relay exits consist of pole149 and beacon152 plus the final auto-walk stop wall.
Sentinel exits instead have exactly one boss61, bridge392, chain339 and terminal240,
with empty terrain below the bridge so collapse can complete. These stages use
CASTLE; Tidal Observatory uses UNDERWATER; other stages use OVERWORLD. CAMERA_MAX
is the absolute right boundary, not scroll distance. PlayerSystem::reset refreshes
water mode on every retry/transition; damage that only shrinks a player keeps it.
Surface-airlock sequences explicitly disable swimming.

## Authoring and validation

The committed campaign directory contains 192 CSV layers,32 properties and a
manifest. tools/build-campaign.py reproduces them without RNG or upstream maps.
The installer runs after all reference/raster work to prevent overwrite.

Run python3 homebrew/starbound-rebased/tools/build-campaign.py, then
python3 homebrew/starbound-rebased/tools/check-campaign.py and
python3 homebrew/starbound-rebased/tools/test-campaign.py.

Checks cover distinct geometry, dimensions, supported IDs, safe spawn/pads,
sequential transitions, enemy support, lifts and complete relay/boss assemblies.
Dry routes use pinned jump/gravity constants with platform boarding/phase/ride/exit
edges; water routes use body-clearance corridor search. Both sizes are checked
across all 32 stages, and dry traversal must fail with movement disabled. The
phase graph is not a frame-perfect full-game simulator and excludes enemy AI.
Compiled tests execute the real factory, PhysicsSystem and PlayerSystem transport
block for both axes/sizes/orderings, endpoints, jump-off, side contact and cycles
outside the viewport. PS5 timing and controller feel still require hardware tests. Both native/desktop jobs
also execute the actual patched C++ property parser. Packaged CSV/properties,
backdrops and new enemy rectangles are verified before release. TESTBUILDS opts
in to this title's prereleases while excluding drafts/other-app prereleases.

## PS5 acceptance

Test1-1 basics;4-2 lifts/sentries;5-1 swimming and damage;5-4 to6-1 mode change;
checkpoint deaths both underwater and dry;6-4 and8-4 cut-off terminals; large-body
clearance; reserve cells; final confirmation. Observe ray/jelly/sentry animation
and confirm the original walking/growth fixes. Automated compilation and static
geometry checks do not replace these hardware tests. Keep v0.1.20 for rollback.

See DESIGN_RESEARCH.md for primary sources and ASSET_NOTES.md for prompts,
normalization and exact artwork addresses.

## Encounter density

There are 421 authored enemies,10–19 per stage. Ordinary encounter bays receive
approach/exit patrols; optional shelves have guards. Boarding/landing zones, entry
supplies and service pads retain safe buffers. All counts and support geometry
are checked against the CSV maps; metadata alone cannot satisfy the checks.
