# PS5MesenX v0.1.15 – HD texture memory and CPU optimization

Experimental PS5 HD-pack performance build; requires physical-console verification.

## What changed
- Preserve full HD pack effects and the original NES emulation logic.
- Decode the pack PNG atlases using the existing bounded two-worker preload.
- Avoid eagerly allocating and copying **every HD tile** from the decoded PNG sheets.
  MesenCE's existing GetMatchingTile() initializes the tile on first actual use.
  This is intended to reduce memory spikes and launch time for large packs.
- Cross-compile the core with -O2 instead of -O1; build HD rendering hot paths
  (HdNesPack, HdVideoFilter, HdNesPpu, VideoDecoder, Libretro) with -O3.
- Leave Safe mode ON by default, preserving the verified smooth non-HD mode.
- Existing bounded catch-up loop and performance logging remain enabled.

## What to measure
Turn off Safe mode for the specific Zelda ROM, enable HD Packs, restart the game.
Record PERF_LOAD_DURATION_MS, PERF_FRAME_SLOW and actual perceived FPS. Large
texture sheets can still be expensive on PS5; this update is not yet
hardware-verified. If the console becomes unresponsive, use Safe mode.

ROMs: /data/PS5NES/games and /data/homebrew/PPSA99896/roms
HD packs: /data/PS5MesenX/HdPacks
Logs: /data/PS5MesenX/logs/boot.log
