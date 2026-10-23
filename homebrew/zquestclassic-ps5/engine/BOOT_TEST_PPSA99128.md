# ZQuestClassic PS5 — original `zplayer` boot diagnostic

**Separate title: `PPSA99128` — EXPERIMENTAL / not playable**

The public `PPSA99127` v0.1.5 quest browser already recognizes and fully
reads `255example.qst` and `the_deep.qst`, and renders the native indexed
palette diagnostic on real hardware. **Do not replace that working title.**

The latest genuine upstream `zplayer` cross-build
([Actions run 39764121870](https://github.com/Yahballon/tmp_lap/actions/runs/39764121870))
now links and converts to an FSELF. That proves the compiler, target linker
and module-conversion tools can process a full original-engine binary.
**Neither console boot nor gameplay has been established.**

The new workflow
[`ZQuest Original ZPlayer Separate PS5 Boot-Test ZIP`](../../../.github/workflows/zquestclassic-zplayer-boot-test-package.yml)
takes that known-good signed FSELF and packages it under the distinct title
`PPSA99128` with PS5 DevBridge's actual `libc.prx`, generated launcher
icon and `sce_sys/param.json`. It does **not** change `version.json`, does
not publish a playable release and does not bundle copyrighted quests.

## Using the experimental package

1. Download the **artifact** named
   `ZQuest-PPSA99128-ORIGINAL-ZPLAYER-BOOT-TEST-UNVERIFIED` from the
   successful packaging workflow run in GitHub Actions. Inside is the
   file `PPSA99128-ENGINE-BOOT-TEST.zip`.
2. Extract/install **PPSA99128**, not PPSA99127. Use your existing
   jailbroken-console homebrew installer that accepts PPSA folders.
3. Run the **ZQuest ENGINE BOOT TEST** title. It may exit/crash before any
   graphics, because the upstream resource/config directory adaptation is
   not complete. This is a *boot-phase* diagnostic only.
4. Inspect via FTP or file manager:

   - `/data/ZQuestEngineBoot/logs/boot.log`
   - `/data/ZQuestEngineBoot/logs/crash.log`
   - Fallback `/data/ZQuestClassic/logs/engine-boot.log`

5. Report the last `BOOT_LOG_READY_AFTER_INIT_ENV`,
   `STATIC_CTOR`, `ZPLAYER_MAIN_ENTER`, `BEFORE_ALLEGRO_SETUP`,
   `ORIGINAL_PLAYER_LOOP_ENTER` or `FATAL_SIGNAL` line, if present.
   Those identify exactly which stage the real engine reached.

**Risk:** This early boot test can fail, hang or crash, and in abnormal cases
may require restarting the Homebrew loader. Do not treat a title tile, black
screen, startup banner, signed FSELF or log entry as gameplay success.

## Engineering limitations

- Console-side runtime, C++ exception/unwinder behavior, imported symbols,
  audio/video event loop, packaged original engine resources, and `-standalone`
  quest argument passing remain to be verified.
- Original ZQuestClassic code stays at upstream commit
  `c44dde7d0146da78bf8b541af457d5ff4e67a830`, under GPLv3; PS5
  adaptation scripts and packaging workflow live in this public repository.
  See both corresponding source trees when redistributing test binaries.
- A first **playable** release still requires actual quest loading, movement,
  ZScript, sound, save/load and clean exit on PS5.
