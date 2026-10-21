# Open Surge PS5 v0.2.2 — Movement and performance fixes

After the user's real firmware 5.10 v0.2.1 video showed correct terrain but sluggish controls:
- Build a 128-world-unit spatial cell index for original solid/cloud collision bricks instead of checking all 1,323 tiles per player-probe pixel.
- Run physics and input through fixed 120 Hz simulation steps even if rendering takes longer.
- Pace display frames with the remainder of 16.67 ms; remove the unconditional 16 ms wait after GPU swap.
- Tune walking acceleration, friction and jumps, keeping the original art, sprite UVs, masks and source game files.
- Display measured FPS and VX; record PERF sampling in /data/OpenSurge/legacy-port.log every ~5 seconds.
- CI: host smoke test of the exact native class with original demo level data, plus a signed PS5 ZIP validation.

This remains a native preview and is not the original Open Surge/SurgeScript game engine. The user must still verify v0.2.2 movement and frame pacing on PS5 hardware.
