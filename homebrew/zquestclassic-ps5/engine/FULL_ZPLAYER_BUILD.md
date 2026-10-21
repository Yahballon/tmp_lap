# Original `zplayer` PS5 cross-build attempt

This is the first workflow that explicitly configures and attempts the **entire
original upstream CMake target `zplayer`** with the PS5 compiler, rather than
only individual engine modules. It is a diagnostic experiment and is not
yet a playable release.

- ZQuestClassic pinned revision:
  `c44dde7d0146da78bf8b541af457d5ff4e67a830`
- PS5 SDL2 fork pinned revision:
  `ee4c47dc0d617b3bc8f35108f9956baf228a1322`
- `JIT_BACKEND=none`: prefer interpreted script execution
- `ALLEGRO_SDL=ON`: ask upstream Allegro fork to use SDL where possible
- `CMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY`: the PS5 binary cannot
  run on the Ubuntu CI worker
- Use PS5-DevBridge Clang wrapper, genuine SDK and FreeBSD-like target
  sysroot (PS5 is not ABI-equivalent to generic FreeBSD; this is **only**
  a starting point and failures must be resolved, not hidden).

Run [ZQuest Original ZPlayer PS5 Cross-Compilation Probe](../../../.github/workflows/zquestclassic-full-zplayer-cross-probe.yml)
in GitHub Actions on the work branch. `configure.log`,
`compile.log`, and `results.txt` are uploaded even when the job fails.
A red job means the **real** engine is blocked; do not label any artifacts
playable.

## Current full-target cross-build diagnostics (PR #204)

The original `zplayer` CMake target has progressed through **1,037 of 1,039
Ninja steps** in [CI run 39642073360](https://github.com/Yahballon/tmp_lap/actions/runs/39642073360).
The original engine sources compile, but `libzcsound.so` failed to link:
`-lm`, `-lusbhid`, `-lpthread` and `-lallegro_dialog` came from
desktop/BSD linkage settings, not proven PS5 runtime dependencies.

The next experimental native cross-build changes **only build configuration**:
- `zcsound` is a static library, following the original Emscripten
  source-path strategy for a single final ELF (no shared `.so` deployment)
- Unsupported desktop native-dialog target is excluded from the
  `allegro_with_legacy` link interface **only in the PS5 probe**
- The pinned OpenAGC SDL2 fork now selects its actual `PS5` platform
  branches (native video, audio, pad, filesystem) rather than generic
  FreeBSD drivers; all such platform calls remain real symbols and must
  resolve at link/runtime

**This is not yet proof that PS5 SDL2 compiles or functions, or that the
original `zplayer` binary links or runs.** Do not substitute dummy video,
audio or input implementations. On-console validation is mandatory.

## Current proven on hardware

v0.1.5 in TESTBUILDS: both quests discovered and fully read, and the
indexed-palette proof texture rendered. No original zplayer gameplay
has ever run in that public binary.

## Conditions for actual finished game

1. Full original `zplayer` cross-configures and cross-compiles with
   proper Allegro+SDL2/PS5 adapters (graphics, input, audio, timers,
   filesystem), and its runtime assets legally packaged.
2. The tested original quest file is passed to upstream `zplayer`
   through its real `-standalone` command-line contract.
3. On PS5 hardware: actual loaded quest graphics, hero movement, input
   events, music/effects, ZScript actions, save/resume and error paths.
4. Distribute `GPLv3` sources and licenses correctly. Only when tests
   above pass should the release be called **playable**.

The existing v0.1.5 TESTBUILDS artifact is intentionally unchanged
by this diagnostic workflow.
