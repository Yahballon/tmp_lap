# Starbound Rebased v0.1.23 — Moving Frontiers TESTBUILD

- 10–19 enemies in every stage: 421 authored spawns across 32 levels, with approach
  patrols, exit patrols, elevated guards and denser submerged encounters. Entry,
  checkpoint, boarding and landing buffers remain clear.
- 54 real moving platforms. All 28 dry stages require boarding a lift, riding
  upward and jumping onto a high ledge.12 cargo/storm/core stages additionally
  require a horizontal ferry across a 28-tile fissure.
- Replaced static 609 campaign tiles with genuine three-tile 809 platform entities.
  Also repaired the engine's one-tile609 factory for future compatible maps.
- Platforms clamp at their endpoints and keep cycling off-camera so a missed
  ferry returns. Horizontal shuttles move faster; vertical lifts remain gentle.
- Player transport uses actual displacement and top contact. Side contact no
  longer drags the player, and descending lifts retain grounded jump input.
- Continuous native48px deck artwork makes the entire96px landing area visible.
- Route checks require dynamic boarding/ride/exit paths and reject dry-level
  completion with movement disabled. Compiled engine tests cover both axes,
  both player sizes, entity ordering, reversals, jump-off and off-camera cycling.

Controls, repaired walking/growth sprites, biome backdrops, underwater mode,
checkpoints and sentinel shutdowns are retained. Install PPSA99642.zip through
ShadowMount; keep v0.1.22 for rollback. Hardware-test boarding, waiting for the
ferry to return, large-player headroom, descending-lift jumps and enemy pacing.
