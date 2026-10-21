# ZQuest Classic PS5 v0.1.4 — Real .qst File Read Test

**Experimental native PS5 TESTBUILD · by YAHBALLON · PPSA99127**

**NOT PLAYABLE:** The original ZQuestClassic `zplayer` game loop is not
integrated. This release adds a real native PS5 **quest-file I/O test**, not a
quest parser, graphics engine, or game.

## New in v0.1.4

- **Cross (X)** now uses `sceKernelOpen` / `sceKernelRead` /
  `sceKernelClose` to read the selected `.qst` file, starting from the
  same kernel-backed quest browser that worked in v0.1.3.
- Nonblocking incremental reading (32 KiB per frame); dynamic progress
  against the detected file size.
- Shows read success/failure, file header bytes and streaming CRC32; uses
  bounded memory and never writes into the quest.
- Logs `QUEST_READ_START`, `QUEST_READ_OK bytes=... crc32=...` or
  `QUEST_READ_FAIL code=...` under
  `/data/ZQuestClassic/logs/port.log`.
- Host POSIX and mocked native-syscall regression tests for file reading
  and known CRC32, plus PS5 native build and package validation.

## Test on your PS5

Leave your two files where they already work:

```text
/data/ZQuestClassic/quests/255example.qst
/data/ZQuestClassic/quests/the_deep.qst
```

Open the app, select a quest, and press **X**. A native read test should
complete and display `READ SUCCESS`. If the app cannot read the bytes,
send us the log containing `QUEST_READ_FAIL`.

**READ SUCCESS does not mean the quest is format-valid or playable.**
This is an explicit diagnostic milestone while porting the real GPLv3
ZQuest Classic engine from ZQuestClassic/ZQuestClassic.
