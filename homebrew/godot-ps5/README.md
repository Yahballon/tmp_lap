# Godot PS5 Probe – TESTBUILDS v0.1.0

A native **PlayStation 5 homebrew hardware diagnostic**, distributed as an explicitly allowed prerelease in `version.json`.

- **Title ID:** `PPSA99842`
- **Release:** [godot-ps5-probe-v0.1.0](https://github.com/Yahballon/tmp_lap/releases/tag/godot-ps5-probe-v0.1.0)
- **Install ZIP:** [PPSA99842.zip](https://github.com/Yahballon/tmp_lap/releases/download/godot-ps5-probe-v0.1.0/PPSA99842.zip)
- **Engine status:** NOT INTEGRATED. This app does not play Godot games.

This diagnostic checks display, DualSense polling, and Godot 4 PCK detection using
PS5 libkernel file operations in `/data/GodotPS5/games` and USB `godot` directories.
Use Square to scan, X to check the selected PCK header, Circle to exit.
Inspect `/data/GodotPS5/probe.log` for details.

The incomplete actual engine port lives in [PR #207](https://github.com/Yahballon/tmp_lap/pull/207), separate from this TESTBUILDS release metadata.
