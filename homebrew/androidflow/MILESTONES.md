# AndroidFlow — Milestones, Evidence & Agent Handoff

**Project:** Yahballon/tmp_lap/homebrew/androidflow
**Last updated (UTC):** 2026-11-03
**Active branch:** feature/androidflow-hydroid-coverflow (commit history in PR)
**Current version / Title ID:** cursor/crash diagnostic v0.1.2 / PPSA99920 (local conflict check only, not globally reserved)
**Hydroid source pin:** d8371afd75e3da6ab888d467d1e6bb00c32d2beb
**Verified native PR build:** GitHub Actions 41596070059 (successful); **Published Release:** not yet; **PS5 hardware:** untested
**Hardware:** PS5 test not performed.

> CURRENT STATE: AndroidFlow's Hydroid source patch and new Coverflow frontend are committed. Host overlay and full native CI passed. GitHub Actions run 41596070059 successfully cross-compiled Hydroid, Dynarmic, FFmpeg and RADV, signed eboot.bin, validated and uploaded PPSA99920.zip. No PS5 hardware test performed.

## Milestone Matrix

| ID | Acceptance criterion | Evidence/status |
| --- | --- | --- |
| M0 | Source reconnaissance and licenses | Hydroid README/BUILD/PORTING, game_file and app_main; GBAXY fe_coverflow_layout.h read — DONE |
| M1 | Native Coverflow scanner UI and select-to-run patch | native/yah_coverflow.c/h, tools/apply_overlay.py — IMPLEMENTED, UNVERIFIED |
| M2 | Host geometry test and upstream patch validation | GitHub Actions run 41590133362 — CI VERIFIED |
| M3 | Native cross-compiled eboot.bin and ZIP | PASS — GitHub Actions run 41596070059; artifact 13212299750, native zip and SHA256SUMS verified |
| M4 | Real PS5 APK launch with screenshot/log | NOT TESTED |
| M5 | TESTBUILD GitHub release and generated version.json catalog update | PR build artifact exists; main-branch release and generated catalog pending |

## Current Architecture & Technical Anchors

- Hydroid provides game_file.c (library scanner, icons, manifest), app_main.c (run_apk, game data), ui_gl.c (OpenGL UI), ui_font.c and input pad.
- AndroidFlow patches app_main.c to detect the packaged androidflow.flag; it runs a new picker and then hands the selected APK path to the existing Hydroid engine. Non-AndroidFlow upstream titles keep their single-game startup path.
- Source overlay script: tools/apply_overlay.py, strictly pinned upstream revision.
- Visuals: native/yah_coverflow_math.h adapts GBAXY pose; native/yah_coverflow.c draws with Hydroid's UI toolkit.
- Packaged icon: assets/generate_icon.py generates Hydroid title/sce_sys/icon0.png; not AI-generated.
- Persisted per-game content remains managed by Hydroid in /app0/data/games; do not delete it on upgrades.
- Logs: /data/homebrew/PPSA99920/android.log; Hydroid can also expose its optional live log port 9025.

## Change Log — Latest First

### 2026-11-03 — v0.1.2 Fruity Game cursor trails and Play crash diagnostics

- Confirmed PS5 screenshot: Fruity Game renders 60 FPS; tens of ghost cursor arrows remain across portrait menu and black bars. The visible rendering artifact indicates pointer overlay pixels are retained between swaps.
- User reports the AndroidFlow process crashes when Play is clicked. Crash origin remains **UNVERIFIED** until the actual console android.log is obtained.
- Scope-limited Godot pre-frame default-framebuffer color clear with explicit restore of scissor, framebuffer bindings, color mask and clear color. Intended to remove persistent overlay cursor pixels, without touching other engines.
- Fixed input duplication: in touch-cursor mode Cross now generates one touch action only, not simultaneous joypad A and keyboard Enter; R3 toggles cursor/gamepad mode.
- Bounded JNI entry and 120-frame post-touch scene tracing added to distinguish input/JNI failure from game-scene crash.
- Native rebuild and actual PS5 repeat test are pending. Preserve v0.1.1 release and all saves; Title ID is still PPSA99920.



### 2026-11-03 — v0.1.1 Godot touchscreen fix for Fruity Game

- PS5 user reported Fruity Game rendered its menu in AndroidFlow v0.1.0, but Start could not be clicked.
- Audited pinned Hydroid `godot_host.c`: key and joystick events forwarded, touch events not handled. The Godot path in `app_main.c` did not set `pad_touch_events=1`.
- Added `tools/patch_godot_input.py` and native `yah_godot_cursor.h`: Godot 3.2 and 3.5 JNI touch dispatch, real touchpad press/move/release, DualSense right-stick cursor, Cross click/drag and overlay pointer reuse.
- Added C host cursor regression tests and native Godot syntax smoke checks. Version bumped to 0.1.1; Title ID unchanged.
- **On-console confirmation of Godot touch Start: PENDING**. Do not claim input works until user tests the PS5 install.



### 2026-11-03 — Full native PS5 build and package validation passed

- CI build **41596070059** completed successfully on commit `e4c69293583b6a23c52e50841368f11efbcd6635`.
- Real Dynarmic JIT, PS5 FFmpeg, Vulkan RADV and patched Hydroid compiled; PS5 native eboot.bin signed and verified.
- Generated valid `PPSA99920.zip` and `SHA256SUMS`; GitHub artifact **13212299750** uploaded. This is a real PS5 package, not a source-only ZIP.
- Real console launch, APK runtime compatibility and coverflow UI remain **UNTESTED**. GitHub release and catalog are pending main-branch publication.



### 2026-11-03 — Fix actual Dynarmic C++ compile errors from run 41595021651

- Full Dynarmic log points to `src/dynarmic/backend/x64/a64_interface.cpp` lines 407 and 412, where `droid_log` is called but never declared in that C++ translation unit.
- Fix: insert `extern "C" void droid_log(const char *fmt, ...);` before the `Dynarmic::A64` namespace after the pinned upstream PS5 patch has run.
- Implemented in `tools/fix_dynarmic_log.py`, invoked by patched `third_party/build-dynarmic.sh`. This preserves the real shared JIT cache and workshop logging; it does not disable them.
- Added fixture tests for single insertion, repeat safety and upstream namespace drift, plus smoke workflow assertions.
- Full native cross-build and PS5 gameplay **not yet verified**; rerun pending.



### 2026-11-03 — Native Dynarmic JIT error isolated

- Run 41593133790 passed FFmpeg with NASM, full SDK/OpenGL/NDK, then built RADV (release).
- Native Hydroid step failed while compiling Dynarmic at 227/229 objects; upstream `third_party/build-dynarmic.sh` truncated the log to only five lines, omitting the actual compiler error.
- Fix: patch pinned Dynarmic build script to preserve complete `dynarmic-build.log`, emit meaningful compiler excerpts and upload it as a GitHub artifact.
- Move Dynarmic into an independently named early CI step after PS5 SDK initialization, before FFmpeg and RADV builds, to reduce iteration cost.
- A native PS5 ELF and installer ZIP are still not verified.



### 2026-11-03 — Fix missing NASM in FFmpeg configure

- Workflow run 41592508086 successfully passed the PS5 cross-link preflight added in the preceding commit, plus Vulkan SDK setup, OpenGL SDK and NDK r27c.
- FFmpeg configure progressed through compiler/CPU checks but then failed with `nasm: command not found`. This is the exact reported error from `ffbuild/config.log` (artifact 13211440825).
- Fix: install NASM on the GitHub runner; validate its presence/version in the initial dependency step and before configure. Preserve FFmpeg x86 optimized assembly instead of disabling it.
- Remaining native PS5 ELF/ZIP and hardware tests unverified.



### 2026-11-03 — FFmpeg configure failure (native run 41590906971)

- **Confirmed:** PS5 SDK, native signer, RADV release, OpenGL SDK and Android NDK r27c all succeeded in workflow run 41590906971.
- **Failed:** `Build native PS5 FFmpeg dependencies`: FFmpeg reported `bash .../ffmpeg-ps5-cc.sh is unable to create an executable file`; no FFmpeg native libraries or PS5 AndroidFlow ELF produced.
- **Fix attempt:** align the linker search PATH with the known ProsperoEden PS5 FFmpeg wrapper; execute a standalone PS5 cross-link probe; print and upload FFmpeg `config.log` on failure.
- **CI optimization:** FFmpeg compile now runs before the long RADV build, so an FFmpeg failure is diagnosed without recompiling the graphics stack first.
- **Validation:** This fix requires a new CI run; does not imply native success or actual console gameplay.


### 2026-11-03 — First native CI failure diagnosed and official zlib mirror retry

- Build run 41590333355 reached native PS5 SDK setup; pinned Hydroid clone, the source overlay, geometry test and SDK fork checkout succeeded.
- Failure: tools/setup-native-dependencies.sh attempted to download zlib 1.3.2 from a GitHub release mirror and wget exited 8 (HTTP failure); no PS5 ELF was built.
- Fix: pre-download zlib from official zlib.net with retries and strict checksum bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16. Keep fallback URLs only if the same checksum validates.
- Added early native C frontend syntax check and Python tool syntax checks to the smoke workflow.
- Hardware: NOT TESTED. Native rebuild result pending.

### 2026-11-03 — Add complete native build attempt and catalog integration

- Confirmed overlay smoke check CI run 41590133362: PASS. It did NOT compile the PS5 title.
- Added missing native RADV isolator (Hydroid's title/build-title.sh referenced title/isolate-radv.py, absent upstream), corrected pinned SDK/CMake and zlib layout paths.
- Added FFmpeg PS5 cross-compiler/wrapper and build_ffmpeg.sh for real media dependencies, with no video-codec stubs.
- Added full native CI build-androidflow.yml (PS5_Vulkan/RADV SDK, Android NDK r27c, Dynarmic, ASTC, OpenGL SDK, FFmpeg), signed package validation, immutable release gate.
- Added tools/package_native.py with SELF magic check, title/Content IDs, 512px icon, fonts, libc.prx, support libs and SHA256SUMS validation.
- Registered AndroidFlow with automatic update-version-catalog.yml workflow_run subscription; version.json remains generated, not hand-edited.
- Observed catalog PR validation failure in run 41590133371 (missing workflow subscription), then patched subscription; revalidation pending.
- **PS5 hardware:** NOT TESTED. **Native cross-build:** NOT YET VERIFIED.

## Change Log — Earlier

### 2026-11-03 — Experimental APK Coverflow integration

- Added app identity separate from GBAXY and other homebrews; checked tmp_lap existing IDs, avoided PPSA99858 (4Wins).
- Added Android library scan, icon cards, controller navigation, animated shallow-angle layout, launch-path selection.
- Added opt-in boot patch for pinned Hydroid: changes neither upstream repo nor other Yahballon apps.
- Added README, native artwork-generation script, smoke-check CI and host geometry tests.
- Validation: Pending GitHub CI; no native compile or console test attempted.
- Regression risk: GL context release before game renderer; USB access, synchronous archive scan, high file counts; C runtime ABI; no in-game return-to-library.

## Known Limitations

- This branch does not contain a PS5 executable or usable TESTBUILD ZIP.
- Real ARM64 game execution has not been confirmed with this integration.
- Caps library at 64 games; only a single directory level per scan; USB path availability varies.
- Opening a new game currently requires relaunching AndroidFlow.
- GPL obligations: preserve Hydroid and GBAXY provenance and all third-party notices.

## Roadmap — Prioritized

P0: Run build-androidflow.yml in PR #298, inspect first failing native-build step, fix and rerun until signed PPSA99920 ZIP and checksums are verified. Review GPU SDK and NDK dependency downloads and publish only from passing main build.

P1: Install on real PS5, scan two owned ARM64 games, test rescan, startup, controls and exits, collect crash logs and actual screenshot in screenshots/1.png.

P2: Optimize background library scan, GPU texture eviction, favorites and safe return-to-shelf.

## Build / Install / Test Playbook

Read README.md for pinned clone/patch instructions. Native builder must provide Android NDK r27c, PS5_Vulkan, ps5-opengl, FFmpeg PS5, Dynarmic and ASTC. Validate title metadata, launcher.flag and sce_sys/icon0.png, import availability and all required native libraries before packaging. Host workflow is not proof of console execution.

## NEXT ACTION FOR THE NEXT AGENT

**Do this first:** Inspect the new native Build AndroidFlow PS5 run after the zlib download repair; the prior run 41590333355 failed before the compiler at zlib setup (wget exit 8). If native setup passes, advance to RADV, Android NDK, FFmpeg and Hydroid compile.

**Starting point:** Yahballon/tmp_lap feature/androidflow-hydroid-coverflow, homebrew/androidflow/, Hydroid SHA d8371afd75e3da6ab888d467d1e6bb00c32d2beb.

**Expected success evidence:** Successful native PS5 cross-build and valid PPSA99920 installation ZIP; actual DualSense selecting an APK and new android.log messages after engine starts.

**If it fails:** Save exact first compiler/linker error or console log, compare unmodified pinned Hydroid to patched tree. Do not make a fake package.

**Do not change:** Original PS5GBAX repo or catalog generator's version.json output, existing Title IDs, or user saves.
