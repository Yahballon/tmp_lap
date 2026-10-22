# Open Surge PS5 – Native compatibility milestones

## Baseline
- **v0.6.3** is the current released baseline. Do not restore pre-0.6.3 collision patches.
- Native PS5 preview currently lacks original Open Surge four-orientation physics and SurgeScript runtime.

## M9a – S-tunnel path fix (candidate; hardware QA pending)
- Video supplied 2026-10-23: tube traversal near world Y 10064 remains unnaturally flat through authored S-shaped level geometry.
- Root cause: native `tick_tube()` interpolated from Tube In to Tube Out in a straight line, rather than original engine tube logic (which only locks input while retaining real rolling physics).
- Branch implementation samples the authentic mask surface at 4-pixel X intervals, accumulates a constant-speed arclength track, and aborts invalid tracks. Retains the original safe-exit guard and normal input lock.
- **NOT validated** against staged actual level assets or PS5 hardware yet. Host build, stage route tests and video regression still required.

## M9b – Looping and original MM_* physics (OPEN)
- Video: ~5–8 seconds fails to preserve continuous motion around the loop.
- Root cause: `LevelPreview::loop_mode_ready()` explicitly returns false. Original `physicsactor.c` uses `MM_FLOOR`, `MM_RIGHTWALL`, `MM_CEILING`, `MM_LEFTWALL`; current preview only follows downward floor normals, treating wall/ceiling as barriers.
- Do not fake the loop with arbitrary teleport/vertical boosts. Next agent should port real rotated collision sensors / angle transitions and their contact-to-air conditions, test against pinned original mask, and preserve v0.6.3 recovery safeguards.
- Tests needed: complete clockwise and reverse loops, low-speed fall-off, S-shaped tube both directions, seamless layer switching, no body penetrations, original 120Hz simulation, no checkpoint or music regression.

## Build / publishing
- Experimental changes live on `fix/opensurge-s-tunnel-loop-v064`; **no v0.6.4 release** until tests and real-console verification.
- Run native PS5 legacy build workflow via PR; inspect `native_routes_smoke`, `native_tube_safety_smoke`, `native_terrain_stress`, and `native_uphill_smoke`.
- When ready, bump app metadata, `main.cpp` version, workflow release tag, version.json and release notes together; publish as explicitly labeled PS5 native preview, never as full engine.
