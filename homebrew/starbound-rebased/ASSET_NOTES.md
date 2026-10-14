# Deep Signal artwork

New raster artwork was generated with the built-in ImageGen tool, then split and
normalized with the sprite-pipeline shared-scale/bottom-anchor workflow. The
committed production assets live under `hd-assets/deep-signal/`; CI does not
depend on an image API, local generation directory or a user's external pack.

## Final prompt set

1. **Enemy animation sheet:** The existing pink/cyan enemy atlas is a palette
   reference only. Generate four rows of four consistent left-facing animation
   frames on transparency: bioluminescent alien ray, pulsing jelly drone,
   mechanical diamond sentry and one connected reactor-sentinel mech. Preserve
   proportions, direction, readable eye, palette and scale within each row.
   Crisp pixel clusters; no labels, scenery, borders or third-party characters.
2. **Biome backdrop sheet:** Three landscape panels on a 1024x1536 canvas:
   submerged teal observatory with coral/light shafts, violet crystal quarry,
   and indigo reactor chamber with magenta rings/cyan cores. Calm dark upper
   strip for HUD, distant layered scenery, empty gameplay reading space. No
   gameplay platforms, characters, collectibles, text or Nintendo shapes.

The final backdrops are 800x480 indexed PNGs with 256 colors, using nearest-neighbor
format normalization. The four species each have four 64x64 transparent source
frames. Packing uses one scale per species and a bottom-center anchor into the
engine's exact native rectangles:

| Species | Native IDs | Frame size | Behavior |
| --- | --- | --- | --- |
| Glow ray | 81/82 | 16x16 | Horizontal swimmer |
| Jelly drone | 48/49 | 16x32 | Pulsing pursuit bursts |
| Air sentry | 90/91 | 16x16 | Animated horizontal flight, stompable |
| Reactor sentinel | 61/63/65/67 | 32x32 | Bounded patrol, jumps and projectile attacks |

These types are newly introduced to Starbound's campaign using existing engine
behavior families. The sentinel's movement was repaired and constrained to its
arena. Defeat is achieved by reaching the marked cut-off terminal, not by an
invented HP mechanic. The original code's fireball HP branch is unimplemented.

The native enemy canvas remains 562x242 with offsets (1,1) and a 16px pitch.
Biome terrain extends the existing code-generated tile artwork in compatible
alias cells. The player source atlas, walking repair, growth frames, projectile
slots and all controller mappings are preserved. Frame previews were visually
inspected; PS5 animation/timing acceptance is still required.
