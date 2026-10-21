# Open Surge PS5 v0.2.4 — continuous DualSense input fix

Real firmware 5.10 testing of v0.2.3 showed stable 59.6 FPS and almost no terrain-block events, yet movement stalled and jumps repeated. The new investigation found that `scePadRead` returns only *new* samples, and the native code erroneously cleared held buttons on every empty read. This repeatedly interrupted RIGHT and retriggered any held face-button.

- Preserve button and stick-held state when no new pad sample is available.
- Only release on an explicit release/disconnect event; preserve short taps from multiple buffered packets.
- Prioritize D-pad direction over contradictory left-stick direction.
- Run host C++ regression: 600 consecutive render frames with sparse pad packets must preserve RIGHT in every frame and generate zero fake CROSS presses.
- Add `PAD_DIAG`, `PAD_CROSS_EDGE` and `PAD_SQUARE_EDGE` logs so future input failures can be distinguished from map collision.
- Retain original 0.2.3 terrain, physics prototype, graphics, asset licensing, and reliable native PS5 OpenGL pipeline.

Still an **experimental level preview**, not the complete original Open Surge/SurgeScript game. Console confirmation required.
