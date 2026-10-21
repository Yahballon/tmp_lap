# 4 WINS v0.1.2 — Visual & Navigation Overhaul

- Replaced the incorrect neon vector background with the provided cinematic neon gaming-room photograph and a clean, empty 7×6 board.
- Rebuilt the UI using Yahballon/ps5-homebrew-ui's input tracker, focus states and SDF renderer. No duplicated/bleeding labels.
- Fully navigable local 2-player or solo bot mode selection, Easy/Medium/Hard difficulty, settings, pause, results, restart and exit.
- Dynamic glossy chips, drop animation, column selector, win-line highlights, bot thinking status, scores and persistence.
- Added `controls_test.cpp` regression tests.

**PS5 Title ID:** PPSA99858 (same as v0.1.0). Install `PPSA99858.zip` from this release over the old version. On-console verification is pending.

## v0.1.2 UI fixes
- Precisely centered live discs and falling animations in all 42 photographed holes.
- Removed pre-rendered UI text from staged assets, plus full opaque masks preventing ghost controls.
- Separated Home, game, paused/result and configuration screens so they never overlap.
- Improved matching and symmetry of bottom player cards and preserved DualSense input.
