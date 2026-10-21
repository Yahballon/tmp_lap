# ZQuest Classic PS5 v0.1.3 — Native Directory Read Fix

**Experimental TESTBUILD · by YAHBALLON · PPSA99127**

The PS5 logs confirmed that `sceKernelOpen` now opens
`/data/ZQuestClassic/quests`, but `sceKernelGetdents` returns
`0x80020016` (EINVAL). The other missing search directories return
`0x80020002` (ENOENT), which is expected when those folders do not exist.

## Changes
- Use the 64-KiB (`0x10000`) directory-entry buffer and 16-KiB
  (`0x4000`) alignment used by the working PS5GBCX directory shim.
- Mock native getdents tests now reject a smaller/misaligned buffer.
- Startup log includes `GETDENTS_BUFFER=65536 ALIGNMENT=16384`.
- Retain Title ID `PPSA99127`, the quest locations and existing UI.

## Test
Leave `the_deep.qst` and `255example.qst` in
`/data/ZQuestClassic/quests`. Update with TESTBUILDS v0.1.3,
press Square to rescan, and verify both filenames appear.

If it still fails, send new lines from
`/data/ZQuestClassic/logs/port.log` containing `DENTS_FAIL`,
`QUEST_FOUND` and `SCAN_DONE`.

**NOT PLAYABLE:** This is a PS5 frontend and filesystem scanner test.
The upstream `zplayer` engine has not yet been integrated.
