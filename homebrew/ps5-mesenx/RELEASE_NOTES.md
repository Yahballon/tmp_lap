# PS5MesenX v0.1.17 – Fix crash when starting a NES game

## Regression found from PS5 boot log

The v0.1.16 app scanned all ROMs correctly but crashed on every launch immediately after `WORKER_START` and `GAME_READ_BEGIN`. The previous successful build read the ROM on the main thread. The new worker-based build had moved `ReadRom` into a background thread; PS5 homebrew filesystem access may not behave equivalently on that thread.

## Fix

- Read and validate the complete NES ROM on the PS5 frontend thread before creating the emulation worker.
- Move the already-loaded ROM bytes into the worker; libretro core init/load/run/unload/deinit stay on one dedicated worker.
- Log `GAME_READ_MAIN_BEGIN`, `GAME_READ_MAIN_OK`, and `WORKER_INPUT_READY` to distinguish file access failures from core startup failures.
- Return a visible error instead of launching a worker if ROM reading fails.
- Keep the existing Safe mode and optional HD-pack behavior, Vulkan rendering and settings unchanged.
- Add build-time checks ensuring the worker no longer invokes `ReadRom`.

**Status:** Native build checks are automated, but the crash fix needs verification on an actual PS5. If it still crashes, send the end of `/data/PS5MesenX/logs/boot.log`.

ROMs: `/data/PS5NES/games` and `/data/homebrew/PPSA99896/roms`.
