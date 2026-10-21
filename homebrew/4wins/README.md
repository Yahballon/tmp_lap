# 4 WINS — PS5 Homebrew
**by YAHBALLON · v0.1.0 · PPSA99858**

A native offline Connect Four game for jailbroken PS5: 7 columns × 6 rows, local two-player mode or solo against Easy, Medium and Hard AI. Four in a row wins; a full grid without a winner is a draw.

## Controls
- D-pad: navigate menus; left and right select columns
- Cross: select, place a chip, rematch
- Circle: return/back, or place yellow chip in local hot-seat mode
- Options: pause; Triangle: restart while paused

## Native build
This project's workflow is `.github/workflows/build-4wins.yml`. It uses the public PS5-DevBridge native template, PS5 OpenGL SDK and generated original raster assets. On successful push to main it publishes `4wins-v0.1.0` containing `PPSA99858.zip`. Install the directory from the validated ZIP under `/data/homebrew/` on a compatible jailbroken console. The TESTBUILDS catalog detects **real GitHub release assets**, not workflow artifacts.

## Assets and screenshots
The game uses procedural neon lounge artwork, plus a blue board and original chip rendering. The user-supplied image remains the visual target for further fidelity improvements. A real PS5 screenshot has **not** been captured.

License: GPL-3.0-or-later; game created by YAHBALLON.
