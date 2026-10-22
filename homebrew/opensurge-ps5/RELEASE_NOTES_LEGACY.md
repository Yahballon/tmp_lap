# Open Surge PS5 v0.6.2 — Four-stage native physics safety update

- Stops the full 120 Hz horizontal substep loop immediately after making first contact with a climbable solid tile. The former logic could keep processing the remaining substeps and then apply gravity through the same tile on the triggering frame.
- Uphill traversal holds X fixed against the solid cliff and checks the next rolling-body position before crest exit. Intermediate contact remains an approximation until the full 4-mode slope sensors are ported.
- Keeps charged Spin Dash velocity stable during wall contact rather than burning friction or restarting a jump mid-climb.
- Avoids applying gravity on the same physics frame as wall climb activation; preserves solid collision and avoids a fall beneath the ramp.
- Repeated automatic falling-respawns are now accurately counted in the existing diagnostic log.
- CI performs the authentic demo screenshot x2500 uphill test with solid-contact checks, **plus** a four-stage terrain movement/dash/fall-recovery stress regression.
- Unchanged from 0.6.1: four stages, original per-stage music, SFX, tube/layer compatibility subset, collectibles, enemies, goal progression, checkpoint and autosave, English UI, DualSense input.
- **Still a native compatibility preview.** Original SurgeScript, Allegro/engine physics, angled loop sensors, full route and enemy logic remain unported. Hardware testing is required.
