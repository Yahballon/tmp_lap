# ThemeChanger v0.3.20 - Experimental PS5 LED compatibility test

This is an opt-in hardware test, not confirmed firmware 5.10 support.

1. Open Signal Companion > Status > Restart ps5-signal. The service should report 0.1.0-tc-compat1.
2. Choose Enable 5.10 LED test (experimental). This stops light effects and saves the current 130-byte LED state before any test colour write.
3. Open Colour and select Blue (255/0/0). Look at the console. The compatibility path changes only existing program level bytes and sends SET directly, without requiring ENABLE first.
4. Return to Status > Restore LED state to restore the saved state and disable the test path.

- The test path requires a recognized three-channel state layout and a durable backup; unknown layouts are rejected.
- On a write or readback failure it attempts to put the immediately previous state back, disables further compatibility writes and returns a failure to the app.
- Saving Colour/Startup configuration is blocked while the UI's test mode is active.
- Hardware IOCTL results/errno and raw before/request/after states: /data/ThemeChanger/signal-hardware.log. Rotation: signal-hardware.log.old.
- Existing command log: /data/ThemeChanger/signal.log. Backup: /data/ThemeChanger/signal-led.before.bin.
- The original path remains the default. No arbitrary IOCTL probing, new kernel offsets, NVS flush or firmware changes are used.
- The custom GPL Signal service is built from pinned upstream sources with SDK v0.43; its build/hash provenance is packaged with the ELF.

The decisive test requires the user's physical console. Matching readback values alone do not prove visible light output. If it fails, send both signal.log and signal-hardware.log; retain the backup file.
