# Open Surge PS5 v0.2.3 — right-only collision and slope fix

Real PS5 video and runtime logs show a stable **59.6 FPS** in v0.2.2, but game movement still pauses even when the user holds only RIGHT: VX drops to 0 at x~723/900 with repeated LAND events.

## Fixes
- Fix the root cause: follow the **original pixel-mask ground surface** when walking, BEFORE classifying the terrain as a wall.
- Track the floor using three foot sensors; smooth uphill/downhill traversal, bounded climb/drop distance per fixed 120Hz step.
- Don't continuously lose and regain grounded status across harmless mask-height transitions.
- Preserve proper body collisions on walls, jump/ceiling checks and original map tile rendering.
- Safely auto-respawn from bottomless demo gaps rather than tracking the character for several seconds through blank sky.
- Periodic PERF log now includes blocked and respawn counters; additional TERRAIN_BLOCK and LEVEL_AUTO_RESPAWN diagnostics.
- CI runs 900 consecutive native C++ frames with **RIGHT held and no jump**, asserting player reaches world x>1000 in the original demo.

Still an experimental level/physics preview, not original SurgeScript gameplay or engine. PS5 hardware re-test required.
