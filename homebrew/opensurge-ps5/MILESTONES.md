# Open Surge PS5 – Native compatibility milestones

## Baseline
- **v0.6.3** is the current released baseline. Do not restore pre-0.6.3 collision patches.
- Native PS5 preview currently lacks original Open Surge four-orientation physics and SurgeScript runtime.

## M9a – S-tunnel path fix (candidate; hardware QA pending)
- Video supplied 2026-10-23: tube traversal near world Y 10064 remains unnaturally flat through authored S-shaped level geometry.
- Root cause: native `tick_tube()` interpolated from Tube In to Tube Out in a straight line, rather than original engine tube logic (which only locks input while retaining real rolling physics).
- Branch implementation samples the authentic mask surface at 4-pixel X intervals, accumulates a constant-speed arclength track, and aborts invalid tracks. Retains the original safe-exit guard and normal input lock.
- Initial host build and prior four-stage stress passed. **But the first full S-tube test still refused the route** (entries=0, refused=1), despite the outlet itself reporting floor=10064 and body_hit=0 at x=4496. Instrumentation now reports the first masked path break so it can be fixed. Hardware QA pending.

## M9b – Authored mask looping preview (implemented, CI / console QA pending)
- Video: ~5–8 seconds fails to preserve continuous motion around the loop.
- Root cause: `LevelPreview::loop_mode_ready()` explicitly returns false. Original `physicsactor.c` uses `MM_FLOOR`, `MM_RIGHTWALL`, `MM_CEILING`, `MM_LEFTWALL`; current preview only follows downward floor normals, treating wall/ceiling as barriers.
- New `LoopRing` discovery detects the original paired Sunshine Bricks 37/38 + 41/42. Authentic SOLID mask probes provide per-angle contact radius; high-speed rolling follows an internally supported ring through FLOOR -> RIGHTWALL -> CEILING -> LEFTWALL -> FLOOR, or the reverse path.
- Speed follows original rolling uphill/downhill incline factors (281.25/1125), with insufficient-speed detach, jump along outward surface normal, and forced exit on damage or springs. No arbitrary 600px/s teleport through the loop.
- Native HUD and `legacy-port.log` report `LOOP_PHASE`, `LOOP_COMPLETE`, and `LOOP_DETACH`. CI adds `native_loop_smoke.cpp` against all four original demo rings in both directions.
- This is still a ring-specific **compatibility prototype**. True general-purpose `physicsactor.c` rotated sensors, integrated SurgeScript, sprite rotation and terrain angular inertia are not yet implemented.
- Next: hardware confirmation of entrance/exit transitions and level ring positions (not just host starting at ring bottom); strict S-tube y-amplitude/exit tests; low-speed/airborne fall-off and jump-angle regressions; save/audio/checkpoint QA; native sprite rotation.

## Build / publishing
- Experimental changes live on `fix/opensurge-s-tunnel-loop-v064`; **no v0.6.4 release** until tests and real-console verification.
- Run native PS5 legacy build workflow via PR; inspect `native_routes_smoke`, `native_tube_safety_smoke`, `native_terrain_stress`, and `native_uphill_smoke`.
- When ready, bump app metadata, `main.cpp` version, workflow release tag, version.json and release notes together; publish as explicitly labeled PS5 native preview, never as full engine.
