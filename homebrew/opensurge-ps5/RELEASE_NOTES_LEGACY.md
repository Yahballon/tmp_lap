# Open Surge PS5 v0.6.4 — S-Tunnel and four-orientation loop QA

Native PS5 compatibility **prerelease**; not a full original Open Surge/SurgeScript engine integration.

- Fix the demo S-tunnel's v0.6.3 horizontal line between authored Tube In and Tube Out. It now follows the original `sunshine_mask.png` terrain contour at 4px spacing and maintains 600px/s along the real arclength, rather than a straight 2D teleport.
- Support both tunnel directions, correct original layer triggers and a bounded 40px seam tolerance. Safety checks still refuse invalid outlets.
- Detect all four original Sunshine ring assemblies directly from Brick 37/38/41/42 and their original per-pixel masks.
- Provide native rolling contact phases FLOOR, RIGHTWALL, CEILING, LEFTWALL, with both traversal directions, normal jumping, insufficient-speed release and uphill/downhill friction.
- Preserve full ring collision silhouette across authored Green/Yellow collision-layer transitions so motion completes instead of clipping on the left-wall sector.
- Add a PS5 HUD loop phase indicator, loop diagnostics, and a SHA256-pinned rapid host physics workflow.
- CI passes eight forward/reverse full ring traversals and exact firmware 5.10 video regression starting at world (5689,9922), speed 597; four-stage 120Hz terrain stress, original steep ramp, forward/reverse S-tunnel and native package tests.
- Preserve existing four-stage selector, music, SFX, DualSense, checkpoint saves, collectibles, entities and hazards.

**Limitations / testing:** Full original Allegro/SurgeScript, general-purpose rotated terrain sensors, sprite orientation, and complete stage-specific scripted gameplay remain unported. PS5 hardware retest is mandatory; this prerelease is a test build, not a claim of feature-complete Open Surge.
