# Signal Companion integration (ThemeChanger v0.3.16)

Open the sidebar and choose **Signal Companion**. Use **L2/R2** for Status, Colour and Startup; **Circle** returns focus to the sidebar. The service is queried only after this page has been opened. Opening the page never starts a payload or changes the console configuration.

## Status

Cross starts the bundled service through the jailbreak ELF loader on localhost port 9021, or restarts an existing compatible service. The status page offers refresh, a beeper test, a blue-light test, stop previews and the optional ps5-autoloader switch. Autoload is changed only on explicit selection, and only when its existing list is accessible.

## Colour

Up/Down selects the control. Left/Right adjusts the blue, white or amber LED channel or the preset/slot. Live preview is opt-in and limited to twice a second. Keep this colour (or Square) persists the mix; Use system colour releases the override. Six personal slots live under /data/ThemeChanger/signal/colours.json. The console has three physical LED channels, not arbitrary RGB output.

## Startup

Triangle switches between beeps and lights. Up/Down chooses a step, Cross chooses its editable field, Left/Right adjusts it, Options adds, R3 removes, Touchpad previews, and Square saves both sequences. Limits match ps5-signal: 24 steps per sequence and at most 10 seconds. Empty sequences disable that startup effect. A colour save preserves startup, and a startup save preserves the kept colour. Saving is blocked until the service config has arrived.

## Provenance and build

GPL-3.0-or-later. Signal Companion backend by Marice, based on ps5-homebrew-ui by BlackBearReloaded, from https://github.com/Marice/ps5-signal-companion at dd34b9917871830547be000fe3e7ae6d932c8abd. The files native/signal/{json,model,service,palette,autoload}.{hpp,cpp} and native/core/save_file.{hpp,cpp} retain upstream notices. ThemeChanger supplies its own panel controller and view.

The build compiles the loopback-only release target of https://github.com/Marice/ps5-signal at 8ed455197a8643302a066b7b0e97972036998304 and includes assets/payload/ps5-signal.elf plus its GPL license. It does not use SIGNAL_DEV or expose the service to the LAN. Source remains available at that pinned public commit. A compatible jailbreak ELF loader is required to start it.

CI checks section-preserving config saves, sequence bounds, offline protection, palette persistence and non-destructive autoload edits against the upstream in-memory service. The native PS5 build and installer asset checks run before publication. Actual console LEDs, beeper, payload handover and firmware compatibility still require a hardware test.
