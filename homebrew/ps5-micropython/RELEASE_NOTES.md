# PS5 MicroPython v0.1.1

Small quality-of-life update based on successful user testing on a PS5.

- Added a permanent bottom-left **ADD YOUR OWN SCRIPTS** help panel.
- Shows the full `/data/PS5MicroPython/scripts/` folder on screen.
- Explains transfer via FTP / file manager and Triangle to rescan.
- Keeps long script lists navigable in an eight-row viewport without overlapping the help card.
- Unchanged MicroPython v1.26.1 embed runtime, Python demos and existing user scripts.

Verified on host: GitHub Actions cross-build and package checks must pass before release.
Previous v0.1.0 user-provided console image demonstrates Fibonacci successfully executed on PS5.
v0.1.1 device verification remains pending.

Limitations: no code editor, no full CPython library and Python execution is synchronous.
