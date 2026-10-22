# ZQuest Engine BOOT TEST v0.0.2 — Persistent startup/crash logs (PPSA99128)

**EXPERIMENTAL / NOT PLAYABLE / PS5 boot still under investigation.**

v0.0.1 was tested on a jailbroken PS5 and immediately crashed with
`SIGSEGV`, fault address `0x0` and `RIP 0x0`. The first reported
backtrace address was `0x9980e3`. This alone does not identify the
specific null call. v0.0.2 therefore instruments the **original**
`zplayer` engine and the original PS5-DevBridge CRT to capture early stages.

## New PS5 log files

- **Startup trace:** `/data/ZQuestEngineBoot/logs/boot.log`
- **Fatal signal trace:** `/data/ZQuestEngineBoot/logs/crash.log`
- **Fallback** (if the separate directory is inaccessible):
  `/data/ZQuestClassic/logs/engine-boot.log` and
  `/data/ZQuestClassic/logs/engine-crash.log`.

Files are written in append mode; existing logs are not removed. The
boot logger initializes immediately *after* the PS5 runtime's `_init_env`
and writes startup progress **before C++ static constructors**. It records
individual initializer positions and addresses, thread start, original
`zplayer` main, Allegro setup, bitmap buffers and original loop entry.
Null entries in the initializer arrays are skipped instead of being called.
Signal handlers attempt to record SIGSEGV/SIGBUS/SIGILL/SIGABRT, then
restore default handling to preserve the console crash report.

Because logging starts after `_init_env`, crashes inside the loader or
before runtime initialization can still leave both files absent. Signal
logging is best-effort and may be preempted by the platform crash handler.
A log does **not** prove the whole engine is bootable or playable.

## Test procedure

1. Install this **separate** PPSA99128 v0.0.2 test build. Keep the
   functional PPSA99127 v0.1.5 Quest Browser installed.
2. Launch the engine boot test once.
3. If it crashes/black-screens, open the log files with PS5-DevBridge,
   FTP or another trusted homebrew file manager.
4. Send `boot.log` and `crash.log` (including the last `STATIC_CTOR`
   or `BEFORE_...` line), together with a screenshot of the console error.
   If the files are missing, say so explicitly.
5. Do not expect `.qst` gameplay yet. The original Quest Browser is not
   connected to this direct engine boot.

No proprietary Nintendo game assets, commercial quest files or ROMs are
bundled. Original GPL-3.0-or-later source:
https://github.com/ZQuestClassic/ZQuestClassic.
