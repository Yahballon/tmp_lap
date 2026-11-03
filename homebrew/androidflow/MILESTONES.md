# AndroidFlow — Milestones, Evidence & Agent Handoff

**Project:** Yahballon/tmp_lap/homebrew/androidflow
**Last updated (UTC):** 2026-11-03
**Active branch:** feature/androidflow-hydroid-coverflow (commit history in PR)
**Current version / Title ID:** source prototype v0.1.0 / PPSA99920 (local conflict check only, not globally reserved)
**Hydroid source pin:** d8371afd75e3da6ab888d467d1e6bb00c32d2beb
**Verified GitHub Release / TESTBUILD:** NONE
**Hardware:** PS5 test not performed.

> CURRENT STATE: AndroidFlow's Hydroid source patch and new Coverflow frontend are committed. Host overlay workflow #41590133362 passed geometry, icon and pinned Hydroid patch checks. Full native build CI is now defined but no signed PS5 executable, ZIP, screenshot or Android game execution has been verified.

## Milestone Matrix

| ID | Acceptance criterion | Evidence/status |
| --- | --- | --- |
| M0 | Source reconnaissance and licenses | Hydroid README/BUILD/PORTING, game_file and app_main; GBAXY fe_coverflow_layout.h read — DONE |
| M1 | Native Coverflow scanner UI and select-to-run patch | native/yah_coverflow.c/h, tools/apply_overlay.py — IMPLEMENTED, UNVERIFIED |
| M2 | Host geometry test and upstream patch validation | GitHub Actions run 41590133362 — CI VERIFIED |
| M3 | Native cross-compiled eboot.bin and ZIP | build-androidflow.yml + tools/package_native.py implemented; CI run and signed package not yet verified |
| M4 | Real PS5 APK launch with screenshot/log | NOT TESTED |
| M5 | TESTBUILD GitHub release and generated version.json catalog update | NOT STARTED |

## Current Architecture & Technical Anchors

- Hydroid provides game_file.c (library scanner, icons, manifest), app_main.c (run_apk, game data), ui_gl.c (OpenGL UI), ui_font.c and input pad.
- AndroidFlow patches app_main.c to detect the packaged androidflow.flag; it runs a new picker and then hands the selected APK path to the existing Hydroid engine. Non-AndroidFlow upstream titles keep their single-game startup path.
- Source overlay script: tools/apply_overlay.py, strictly pinned upstream revision.
- Visuals: native/yah_coverflow_math.h adapts GBAXY pose; native/yah_coverflow.c draws with Hydroid's UI toolkit.
- Packaged icon: assets/generate_icon.py generates Hydroid title/sce_sys/icon0.png; not AI-generated.
- Persisted per-game content remains managed by Hydroid in /app0/data/games; do not delete it on upgrades.
- Logs: /data/homebrew/PPSA99920/android.log; Hydroid can also expose its optional live log port 9025.

## Change Log — Latest First

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

**Do this first:** Inspect the Build AndroidFlow PS5 (Hydroid ARM64) GitHub Actions job for PR #298 and fix the first native compilation/link error; keep M2 overlay validation (run 41590133362) passing.

**Starting point:** Yahballon/tmp_lap feature/androidflow-hydroid-coverflow, homebrew/androidflow/, Hydroid SHA d8371afd75e3da6ab888d467d1e6bb00c32d2beb.

**Expected success evidence:** Successful native PS5 cross-build and valid PPSA99920 installation ZIP; actual DualSense selecting an APK and new android.log messages after engine starts.

**If it fails:** Save exact first compiler/linker error or console log, compare unmodified pinned Hydroid to patched tree. Do not make a fake package.

**Do not change:** Original PS5GBAX repo or catalog generator's version.json output, existing Title IDs, or user saves.
