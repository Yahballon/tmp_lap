# ZQuest Classic PS5 v0.1.2 — Activate PS5 Kernel Scanner

**TESTBUILD · by YAHBALLON · PPSA99127**

Fixes the configuration error found in v0.1.1 console logs: `DIR_FAIL code=1` showed the POSIX fallback rather than a native PS5 kernel scan. The compile-time scanner selection now lives in the app's own main translation unit and does not change graphics SDK build flags.

Changes:
- Enable the native filesystem scanner only in the app's main translation unit.
- Reject any build without the native filesystem backend enabled.
- Verify the source-level native scanner selection and final ELF backend marker.
- Log `SCAN_BACKEND=libkernel-getdents-v0.1.2` at startup.
- Preserve Title ID PPSA99127, save directories, existing icon and quest files.

Expected Quest files: `/data/ZQuestClassic/quests/the_deep.qst` and `/data/ZQuestClassic/quests/255example.qst`.

If files are still missing, send the fresh `/data/ZQuestClassic/logs/port.log` lines, especially the backend marker and `DIR_FAIL` codes. Native kernel access has not yet been confirmed on the user's firmware.

**Still NOT PLAYABLE:** the full ZQuest `zplayer` engine is not integrated. This release tests the library scanner only.
