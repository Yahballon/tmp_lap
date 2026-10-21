# ZQuest Classic PS5 v0.1.1 — Native Quest Scanner Fix

**Experimental PS5 TESTBUILD · by YAHBALLON · PPSA99127**

**NOT PLAYABLE:** This is still the native PS5 quest browser. The original
ZQuest Classic `zplayer` engine is not integrated and quests cannot yet run.

## Fixes
- Replace sandbox-incompatible libc `opendir` / `readdir` with direct
  `sceKernelOpen`, `sceKernelGetdents`, and `sceKernelClose` enumeration.
- Avoid silently hiding `.qst` entries when `lstat` is unavailable.
- Support `DT_UNKNOWN` quest names in the native directory stream.
- Add `DIR_OK`, `DIR_FAIL`, `DENTS_FAIL`, `QUEST_FOUND`, and `SCAN_DONE`
  diagnostics at `/data/ZQuestClassic/logs/port.log`.
- Show folder-access and error counts under Options.
- Add mocked native syscall regression test with
  `the_deep.qst` and `255example.qst`.
- Keep original Title ID `PPSA99127`, persistent data paths and icon.

## Test on PS5
Place both `.qst` files at `/data/ZQuestClassic/quests` as before.
Update from TESTBUILDS and press Square to rescan.
If still empty, press Options and send the scan status and the
`/data/ZQuestClassic/logs/port.log` diagnostics.
The PS5 filesystem may still reject direct kernel access under some
firmware/mount configurations; hardware verification is pending.

## Build and provenance
Native PS5 cross-compilation and executable/ZIP checks run in GitHub Actions.
Upstream ZQuestClassic/ZQuestClassic remains GPLv3 (not yet linked).
No Nintendo assets or commercially copyrighted quests are bundled.
