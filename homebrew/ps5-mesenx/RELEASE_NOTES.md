# PS5MesenX v0.1.14 – Performance Safe Mode

Experimental native PS5 build. On-console performance still needs verification.

- **Safe mode ON by default:** forces HD packs off even if an old per-game setting requests them, to isolate the HD-pack latency. To explicitly test HD packs, disable Safe mode in Settings > Emulation and turn HD Packs On; restart game.
- Bound emulation catch-up to 1 frame/tick with Safe mode (2 outside it) instead of 6; drop overdue backlog so pad and Vulkan presentation are polled.
- Reduce periodic battery-save disk I/O from 5 to 60 seconds; saves still occur on stopping the game.
- Reuse Libretro init instead of deinit/init on every game open; MesenCE options are read again at ROM load.
- Log `PERF_CONFIG`, `PERF_LOAD_DURATION_MS`, `PERF_FRAME_SLOW`, `PERF_UPLOAD_SLOW`, and `PERF_GAME_READY` to `/data/PS5MesenX/logs/boot.log`.

Games: `/data/PS5NES/games` and `/data/homebrew/PPSA99896/roms`. HD packs are in `/data/PS5MesenX/HdPacks`. No ROMs are included. Title ID `PPSA99896`.
