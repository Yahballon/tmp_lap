# ZQuest Classic real zplayer engine: PS5 native bringup

## Current status

**This is NOT a playable zplayer engine yet.** The *released* v0.1.3 app is a
Quest Browser and already detects real `.qst` files. Pressing Cross shows
details, because no `zplayer` game loop exists in the installed executable.

The engine-bringup workflow provides reproducible upstream **C++ compilation milestones**:
the engine's original `src/base/random.cpp` and `src/base/random.h` are
fetched from the pinned ZQuestClassic upstream commit and compiled with the
actual PS5 SDK as part of a throwaway, clearly marked NON-PLAYABLE test
artifact. Its boot diagnostic `UPSTREAM_RNG_NATIVE_SMOKE seed=42 ...` is
linked to the real source, not a stand-in implementation. It proves this
that original ZQuest core code cross-compiles, **not** that ZScript or gameplay
works.

## Pinned upstream

- ZQuestClassic/ZQuestClassic, revision
  `c44dde7d0146da78bf8b541af457d5ff4e67a830`, GPLv3.
- The game is CMake target **`zplayer`**, entrypoint
  `src/zc/zelda.cpp:main`, linked with `core`, `zcsound`,
  `zconsolelogger`, `zcgui`, `worker_pool`, `websocket`,
  `poolSTL_tbb` and `imgui_color_text_edit`.
- The game expects Allegro 5 plus Allegro-Legacy; the normal native CMake
  build uses additional vendored/FetchContent libraries.
- Upstream's real quest argument is
  `-standalone <path/to/quest.qst> [save.sav]`
  in `src/zc/zelda.cpp:init_and_run_main_zplayer_loop()`.
- `standalone_args.hpp` generates those actual arguments and a PS5 writable
  save path for an existing indexed quest. It is a *future player-call bridge*,
  not a launcher yet; it is deliberately not wired to Cross.

## Native ZASM command-table milestone (PR #198)

The native PS5 SDK now also compiles and links the **original**
`src/components/zasm/table.cpp` instruction/variable lookup table from the
same pinned upstream commit. CI runs host tests asserting original ZASM
`WAITFRAME`, `GOTO`, and `SETV` opcodes, and independently validates
`UPSTREAM_ZASM_TABLE_NATIVE_SMOKE` in the PS5 executable.

The PS5-DevBridge native build uses `-fno-exceptions`, while upstream
`table.cpp` contains two *constexpr duplicate-table assertions* implemented
with `throw`. For this PS5 build alone, `prepare_zasm_for_ps5.py` replaces
exactly those two compile-time assertions with `__builtin_trap()`, which
still rejects duplicate opcodes during compile-time evaluation. Host tests
compile the upstream source **unmodified**. No gameplay opcodes are replaced.

**This does not run scripts:** zplayer, the ZASM interpreter, its VM state,
Allegro, the original graphics/audio path and quest logic remain unported.
The production TESTBUILDS `PPSA99127.zip` remains v0.1.4 as a native
file-read diagnostic; the engine bringup ZIP is an isolated CI artifact.

## Engine backend path

An existing PS5 SDL2 fork can reduce platform engineering:
`OpenAGC/SDL2` `release-2.30.x-ps5`, examined at commit
`ee4c47dc0d617b3bc8f35108f9956baf228a1322`.
It contains `src/video/ps5` (framebuffer/output) and `src/audio/ps5`,
along with PS5-specific input code. This may be preferable to hand-writing
every SDL function. **It has not yet been built against the current
PS5-DevBridge toolchain, and SDL2 compatibility does not guarantee
Allegro-Legacy compatibility.**

Upstream does have experimental `ALLEGRO_SDL=ON` handling and the
Emscripten build config uses SDL2; that Web/WASM config relies on browser,
SDL2_mixer and pthread/asyncify facilities and cannot simply be renamed
to `eboot.bin`.

## Concrete remaining work before a playable release

1. Establish **SDL2 PS5 target** compatibility with existing SDK/loader and
   test VideoOut, pad events, audio hardware; identify whether the PS5 fork
   requires a different payload SDK, custom CRT, or additional imports.
2. Adapt/build patched upstream Allegro 5 and Allegro-Legacy against the
   tested PS5 SDL2 platform: 8-bit BITMAP updates, palette, timers, audio,
   joystick/keyboard event pumping and fullscreen/window semantics.
3. Compile original `core`, `zasm` script interpreter, sprites, maps,
   audio and the whole `zplayer` executable. Start with
   `JIT_BACKEND=none` and disable optional network/editor components only
   where the game truly supports this.
4. Connect the selected `.qst` to original `zplayer` via
   `-standalone`, map save/config dirs to writable PS5 paths,
   use original game resources, handle loading/errors without memory leaks.
5. On real console, verify quest selection -> graphics -> hero movement ->
   collision/events -> sound -> saves/reload -> exit. Use a quest you may
   legally redistribute for automated tests. No copyrighted game files
   should be committed.
6. Only after **all** of these tests pass, update the regular PS5 homebrew
   app and publish a clearly labeled **playable** TESTBUILDS version.

## Test commands

Native smoke CI: [ZQuest Classic Real Engine PS5 Bringup](../../../.github/workflows/zquestclassic-engine-bringup.yml).
Run via GitHub Actions. This stages original engine source at build-time; the
GitHub artifact is a TEST-ONLY diagnostic build. It **does not** alter the
existing public v0.1.3 release or `version.json`.

The `standalone_args_test.cpp` is also independently buildable with a
host C++20 compiler; the RNG regression is linked against *pinned upstream*
`base/random.cpp` and is not independently meaningful without that source.

## Copyright & security

The upstream is GPLv3. Preserve original license, history, notices and offer
source when redistributing a finished engine. Do not redistribute Nintendo
graphics/audio/ROMs or quests lacking a license. The diagnostic RNG seed
must not be used as a cryptographic RNG. The bridge refuses unknown filesystem
roots and traversal in quest paths; it must never accept arbitrary network
input as a quest name.
