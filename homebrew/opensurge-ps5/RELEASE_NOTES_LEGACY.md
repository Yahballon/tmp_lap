# Open Surge PS5 v0.5.1 — Spin Dash uphill fix

Hardware screenshots show **60 FPS and VX=0** around world x=2500/y=10015 on the Sunshine slope. This is a mask/body collision problem, not a performance issue.

- Roll uses a narrow and lower ~15x29 collision sensor (as in original physicsactor.c), not the upright character's sensor against a steep hill.
- Derive uphill/downhill forces from original collision mask heights at the player's feet and use original rolling hill strength (281.25 uphill and 1125 downhill world units/s²).
- Keep the earlier v0.2.4 buffered controller fix and 120Hz simulation, native terrain, collectibles, enemies, sound and music unchanged.
- New host C++ regression reproduces the recorded x=2500/y=10015 grounded state, charges a Spin Dash, and asserts actual forward/uphill travel rather than camera-only movement.
- Original physics is still an approximation, full SurgeScript engine remains unlinked.
