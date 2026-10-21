# Open Surge PS5 v0.3.1 - Sonic-style movement preview

User's v0.3.0 test confirmed stable ~59.6 FPS and working pickups/spring but missing Sonic-style movement and authentic player animations.

- DOWN at rest crouches, UP at rest looks upward using upstream frames.
- DOWN while moving starts momentum-preserving roll. Turning against roll decelerates it; does not snap speed back to walk pace.
- DOWN + X begins charge, repeated X adds 0.25 intensity, releasing DOWN launches a Spin Dash (480 to 720 px/s), using upstream physicsactor.c formula.
- Original Surge.spr sprite frame selections for idle/walk/run/duck/look-up/charge/brake/rolling/jumping/falling.
- Fix spring retrigger spam: rearm only after leaving the spring area and passing cooldown.
- Retain PS5 v0.2.4 verified DualSense buffered controls and 120 Hz physics, all original demo objects and renderer.
- CI runs native C++ smoke tests with actual pinned original level data for Spin Dash, reset, collectibles, and spring cooldown.

This remains a native experimental compatibility layer, **not** the original full SurgeScript runtime. No original enemy AI, level flow or complete sound yet.
