# Starbound Rebased v0.1.24 — DualSense Settings TESTBUILD

- Replaced the blocking keyboard-only settings capture with per-frame controller/keyboard capture. Circle cancels capture immediately and returns from settings; Options pauses gameplay and can also cancel a pending capture.
- Cross confirms; D-pad and left stick navigate. Gameplay defaults: Cross jumps, Square fires and sprints, R1 also sprints, D-pad/left stick moves and crouches.
- Settings show real PS5 control names. All six gameplay actions can be rebound to controller buttons or D-pad/stick directions. Release the opening Cross first, then press the new control. Circle and Options stay reserved. Shared bindings are allowed for run/fire.
- Triangle restores controller defaults while browsing settings. Rebindings last for the current game session; reopening settings retains them, restarting the application restores defaults.
- Disconnected controllers release held actions; reconnection scans all available controllers. Keyboard Escape cancellation and keyboard rebinding remain functional on desktop.
- Compiled regression tests exercise the actual Input implementation and settings handlers with a virtual SDL controller, including cancel, opening-button release, remapping, reset, menu/action separation, disconnect and keyboard fallback.

The 32-stage campaign, dense encounters, moving platforms, underwater stages, bosses, HD assets and repaired player sprites are preserved. Install PPSA99642.zip through ShadowMount. Keep v0.1.23 for rollback. PS5 hardware validation remains necessary: test Circle cancel, remap Jump, reset with Triangle, then pause with Options.
