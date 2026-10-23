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

## PS5 libc and Mesa/OpenGL link compatibility

Run [39732505692](https://github.com/Yahballon/tmp_lap/actions/runs/39732505692)
compiled the complete original `zplayer` source tree through the final
link, but showed unresolved SDK/SDL2/Mesa interfaces. After the first
round of corrections, run
[39734560074](https://github.com/Yahballon/tmp_lap/actions/runs/39734560074)
had **only** the three Mesa imports
`mkstemps`, `openlog` and `__emutls_get_address` left unresolved.

The staged PS5-only build now:

- Selects SDL2's existing portable wide-string implementations for PS5
  libc functions that are declared but not exported.
- Links Mihawk PS5's real `ps5_mbrtowc_l` locale function.
- Disables Unix `~otheruser` password-file lookup on a console with no
  passwd database; ordinary `~/`, absolute and relative paths remain.
- Provides a native `mkstemps` backed by `O_CREAT|O_EXCL`, restrictive
  permissions, and a host regression test; Mesa diagnostics go to stderr
  instead of an unsupported Unix syslog daemon.
- Compiles the **original LLVM 18 Compiler-RT emulated TLS runtime**
  (`__emutls_get_address`), retaining pthread-key per-thread storage,
  rather than masking a vital OpenGL function with a zero-address symbol.

These are source and build corrections, **not on-console validation**.
The full player ELF's link status must still be confirmed by the latest CI
run, followed by correct PS5 FSELF packaging, runtime initialization, and
actual quest gameplay testing. Do not put the diagnostic target in TESTBUILDS.

## Original zplayer now links on PS5 toolchain (22 Oct 2026)

**First successful full original player link:** [GitHub Actions 39738727477](https://github.com/Yahballon/tmp_lap/actions/runs/39738727477), pinned upstream ZQuestClassic and SDL2 PS5 sources. Both the CMake compile/link and resulting ELF header checks passed. The genuine `zplayer` binary is archived in the workflow artifacts.

**Not bootable yet:** `llvm-readelf -h` identifies the link artifact as
`ELF64 x86-64`, type `DYN`, but with **entry point `0x0`**. The linker
warned that `_start` was missing. This is not a PS5 SELF/FSELF or a usable
`eboot.bin`. The new `check_ps5_boot_contract.py` reports these precise
remaining blockers in an archived `zplayer-boot-readiness.json`.

Next is integration of the existing PS5-DevBridge `app_crt.cpp` startup
(and its native `_init_env`/runtime), proper PIE linker script, import
module conversion, loader-compatible entry, `-standalone` quest argv and
legally redistributable engine resources. A nonzero ELF entry would not
alone prove that the program can execute on PS5. Console testing of real
video, audio, controls, gameplay and saving must follow.

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
