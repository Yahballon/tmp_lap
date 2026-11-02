# ScummVM PS5 — Milestones, Evidence & Agent Handoff

**Updated:** 2026-11-01 UTC
**Source:** Yahballon/tmp_lap, feature/scummvm-ps5-core
**Upstream frontend:** mihawk-99/PS5_RetroArch @ 7268510b12fe7461342e0cc4497c87a0910c1055
**Upstream engine:** mihawk-99/PS5_ScummVM @ 967fd9757170b14c28590837186b31cc7d5cc46c
**Version / candidate Title ID:** v0.1.0 / PPSA99053, no global reservation
**Release / TESTBUILD:** NONE; generated version.json untouched; latest completed native run 41319819390 FAILED after ScummVM core ABI PASS
**Console test:** NOT PERFORMED; **Screenshot:** NOT CAPTURED

> **CURRENT STATE:** ScummVM PS5 upstream core and PS5 RetroArch sources inspected. A dedicated, ScummVM-only build overlay and CI are prepared for actual native build. No dedicated-title PS5 execution has been observed.

## Milestone matrix

| ID | Goal | Status | Evidence |
| --- | --- | --- | --- |
| M0 | Identify usable PS5 ScummVM core | Source verified | PS5_ScummVM fork and PS5_RetroArch's build-scummvm.sh pinned commits |
| M1 | Dedicated unique-ID ScummVM-only frontend and icon | IMPLEMENTED / UNVERIFIED | scripts/prepare.py plus source artwork; CI pending |
| M2 | Genuine PS5 compilation and signed ZIP | CORE ABI PASS / TITLE BUILD FAILED | Run [41319819390](https://github.com/Yahballon/tmp_lap/actions/runs/41319819390) passed ScummVM core ABI then failed on donor Lapy service dependency |
| M3 | Publish eligible real release and auto-generated TESTBUILDS entry | NOT STARTED | no verified PPSA99053.zip GitHub release |
| M4 | Confirm ScummVM UI, virtual mouse, audio, saves and game data on PS5 | NOT STARTED | user PS5 hardware test needed |

## Architecture

- RetroArch PS5 frontend with just scummvm_libretro.so, built with Mihawk's pinned scripts, plus original native PS5 audio/input/Vulkan backend.
- build-title.sh is patched only in temporary checkout, not the donor repository. No unverified native standalone ScummVM backend is asserted.
- app.json identity: PPSA99053, UP9000-PPSA99053_00-SCUMMVMPS5000001.
- Source: homebrew/scummvm-ps5; workflow: .github/workflows/build-scummvm-ps5.yml.
- Storage: /app0/config and /app0/content from donor's platform; persistence/access on PS5 must be tested.

## Change log — 2026-11-01

- Inspected tmp_lap main, matching apps, 31-entry generated TESTBUILDS catalog, donor PS5 RetroArch build, its ScummVM core script, frontend boot path and package metadata. Searched catalog and available repositories for Title ID PPSA99053; no match found there, broader uniqueness unproven.
- New scripts prepare a pinned upstream source checkout, replace the 33-core build list with only ScummVM, rebrand the title, preserve donor UI/backend, and stage a custom generated icon.
- Added script-driven PS5 packaging validation, host tests and build workflow. Dedicated native compilation and real console test **not performed at this checkpoint**.

## CI evidence and repair — 2026-11-01

- PR #282 workflow run [41289470054](https://github.com/Yahballon/tmp_lap/actions/runs/41289470054), job 124797508023: checkout, Python preflight, pinned frontend/Mesa/Vulkan checkout PASS; `Prepare PS5 SDK and RADV toolchain` FAILED with `the payload SDK fork at .../.work/PS5_PayloadSDK does not have b5efad528e0ac1b8289f39d72a0e890ff68ff0a6` (exit 2). No native compile or ZIP produced.
- Fixed the workflow to clone `mihawk-99/PS5_PayloadSDK`, verify donor-required commits `b5efad5...` and `b83202b...`, pass fork explicitly to bootstrap scripts and prepare native link tooling before RADV. The two projects pin separate SDK revisions; no hash checks disabled.
- **Fix status:** COMMITTED / UNVERIFIED. Inspect next CI run for result.

## Release/TESTBUILD handoff — 2026-11-01

- Added a `publish` GitHub Actions job gated by successful native build on `main`. It downloads the actual verified signed ZIP and checksum, checks contents and publishes `scummvm-ps5-v0.1.0` as an experimental GitHub prerelease. PRs do NOT release.
- Updated the existing catalog generator's explicit prerelease allowlist, catalog workflow_run subscription and host tests. `version.json` itself remains generated; no fake entry is inserted.
- Publication not verified: wait for successful PR native compile and merge. Only then can the publisher run and catalog discover a real artifact.

## Second native CI failure and RADV-only fix — 2026-11-01

- Run [41290073628](https://github.com/Yahballon/tmp_lap/actions/runs/41290073628), job 124799350506: both pinned payload SDKs and zlib compiled successfully after earlier fix.
- The donor's optional PS5_Vulkan demo `make` failed with `undefined symbol: ps5_fp_ieee`, but the required native linker and libc.prx existed; that demo failure is not evidence that RADV or ScummVM failed.
- The real blocking error occurred in RADV setup: `Meson version is 1.3.2 but project requires >= 1.4.0`. The CI workflow now creates a venv with pinned Meson 1.9.2 before RADV compilation.
- Patched the temporary donor build-title.sh to skip checking legacy ps5vk-only archives when the chosen driver is RADV; the actual RADV archive is still independently linked and verified. Prevents a predictable unnecessary demo dependency from blocking ScummVM.
- These fixes are **committed but NOT CI-verified** at this checkpoint; observe the next run's logs and do not claim a PS5 binary yet.

## Native CI evidence — RetroArch stage, 2026-11-01

- Run [41290409229](https://github.com/Yahballon/tmp_lap/actions/runs/41290409229), job 124800344057, completed **failure** at Compile ScummVM-only signed PS5 app. Both pinned SDKs, Meson 1.9.2 and actual RADV release archive completed successfully.
- Exact failing error: `error: run tools/fetch-retroarch.sh first` in the original PS5_RetroArch frontend build; not a ScummVM core compiler error.
- Fixed by invoking donor's `tools/fetch-retroarch.sh` with immutable upstream 1.22.2 pin and `--verify`, setting `PS5_PAYLOAD_SDK` explicitly to the matching frontend SDK, and restricting `generate-core-metadata.py` to the single included ScummVM core.
- Extended the donor manifest integrity checks from FCEUmm-only to ScummVM-only in the temporary checkout; the workflow now executes manifest verification before packaging.
- Added the pinned `PS5_MihawkGen` source checkout plus immutable LLVM licence text required by the donor's legal-notices packaging phase.
- **These changes have not been verified by a complete native build.** Monitor the latest feature branch CI run. No signed ZIP/Release/hardware test may be claimed until the corresponding evidence exists.

## Failure investigation: RetroArch configure — 2026-11-01

- Latest completed native CI: [41308958226](https://github.com/Yahballon/tmp_lap/actions/runs/41308958226), **failed**. Pinned RetroArch 1.22.2 checkout succeeded; RADV release built. At the frontend build: `error: configure failed; see .../build/ra-conf/configure.log`. Actual compiler/configure error was *not emitted* in GitHub job logs.
- To avoid guessing at compiler flags, the workflow now runs RetroArch configure and `make info` as a **fail-fast preflight before RADV** and prints the underlying `configure.log` plus `config.log` on error. A failure-only artifact preserves the logs for inspection.
- **No native PS5 binary, signed ZIP, or TESTBUILD release confirmed.** Next: read the new configure log from Actions, identify the first actual failure, and patch only that cause.

## Root cause confirmed: cross-compile Vulkan detection — 2026-11-01

- Run [41311488818](https://github.com/Yahballon/tmp_lap/actions/runs/41311488818) failed at the early frontend preflight. Real RetroArch configure error: `Forced to build with library -lvulkan, but cannot locate. Exiting ...`; its check is `Checking function vkCreateInstance in -lvulkan ... no`. All other unavailable optional host libraries shown in config.log were not fatal.
- **Fix committed / native verification pending:** Generate a minimal PS5-architecture `libvulkan.a` in `build/configure-vulkan-probe/` for RetroArch's configure test only. Patched temporary donor `retroarch-sources.sh` so only `./configure` receives its `LDFLAGS=-L.../configure-vulkan-probe`; actual title linking remains against the genuine PS5 RADV static driver. No probe objects are shipped in `PPSA99053.zip`.
- Run diagnosis artifact: [ScummVM-PS5-CI-DIAGNOSTICS](https://github.com/Yahballon/tmp_lap/actions/runs/41311488818/artifacts/13085655625).
- Required next gate: confirm configure now passes and PS5 ScummVM core compiles; inspect subsequent native link/package failures as needed.

## ScummVM core ELF ABI normalization — 2026-11-01

- Run [41311904239](https://github.com/Yahballon/tmp_lap/actions/runs/41311904239) now PASSED frontend configure, PS5 SDK/RADV and compiled/linked `scummvm_libretro.so` from PS5_ScummVM commit 967fd975.
- Next failure was **donor core ABI check**: `expected ELF64 little-endian FreeBSD/PS5 ABI, not a host library` immediately after `Linking scummvm_libretro.so...`. This is specifically the 8-byte ELF identification preflight; the full ABI checks never got to run, so exact OSABI is not yet established.
- Added a constrained OSABI normalization helper: only accepts ELF64 LE x86_64 ET_DYN, OSABI 0 or 9, required `libkernel_web.sprx` and exclusively allowed PS5 dependencies, and 16 KiB-compatible PT_LOAD segments. If OSABI is 0, changes only `e_ident[EI_OSABI]` to FreeBSD (9); if 9, leaves it unchanged; if any check fails, aborts without modification.
- Temporary donor `build-scummvm.sh` runs this helper **before** existing `core_fork_stage` and its independent `check-core.py` validation. CI prints ELF before/after and saves binary only on failure. Unit tests cover detection/rejection.
- **Not yet PS5 verified.** Observe next native Actions run. If the PS5 ELF identity or imports are not valid, the checker will reject, and the real link flags must be fixed.

## New failure and standalone-service isolation — 2026-11-01

- [CI run 41319819390](https://github.com/Yahballon/tmp_lap/actions/runs/41319819390): both PS5 SDKs and RADV prepared; ScummVM 2026.3.0 compiled, normalized OSABI 0 -> 9 after checking architecture, imports and segments; donor's strict `check-core.py` reported **CORE ABI PASS**, 25 Libretro exports and SHA-256 `38e3a3a1acba43de7697738f61958b6040a8b222eae29feeef04bd2a38faebe4`. The launcher progressed to ancillary networking, then failed on missing `.work/prospero-win-main/native/ps5log/ps5log.h` while running `tools/build-lapy.py`.
- Donor Lapy service is hardcoded for original title **PPSA99169**, not standalone title PPSA99053; donor `platform/lapy/README.txt` explicitly describes offline startup without loader/service. Rather than bundle an unrelated privileged daemon, ScummVM-only `prepare.py` removes donor Lapy build and staging lines in the temporary checkout. It retains full PS5 core ABI, title manifest and license checks. CI and package.py both reject any unexpected `lapy-root-daemon.elf`.
- Also removed the donor Lapy executable from its inlined Python title build-identity digest input. Otherwise the build would have failed later even though compiling and staging the daemon were disabled.
- **Status: PATCH COMMITTED / BUILD NOT VERIFIED.** The next successful native CI run must validate fake-SELF, core-only contents, icons, licenses and ZIP before prerelease. Console boot has not been tested.

## Risks and unverified behavior

- Upstream build is large (RADV/toolchain/WebUI) and may exceed GitHub runner resources. Check first failure, do not downgrade or mask it.
- `supports_no_game=true` is declared by ScummVM core metadata; dedicated launcher entry into core without content has not yet been demonstrated.
- ScummVM game's .scummvm pointer, mouse controls, keyboard, asset files, saves and USB access need hardware tests.
- The new app's title ID is checked against the known generated catalog, not guaranteed unique worldwide.
- Original repo licenses must be staged and checked inside native package.

## NEXT ACTION FOR THE NEXT AGENT

**First:** Inspect the next `Build ScummVM PS5` GitHub Actions run on `feature/scummvm-ps5-core` after the SDK checkout fix. Validate that the pinned upstream can compile a single-core title, `dist/PPSA99053.zip` exists, `PPSA99053/eboot.bin` is signed, and `sce_sys/icon0.png` matches the generated image. Fix the first actual failure, recording workflow run ID and error here.

**Next:** Verify on a real PS5 that menu starts, Load Core > ScummVM starts without content, an owned game starts with controller/mouse, sound and saves work. Capture real screenshot at `screenshots/1.png`; only then record observed hardware success. When a signed GitHub Release asset exists, let the existing `tools/update-version.py` workflow generate TESTBUILDS catalog automatically.

**Do not change:** existing app title IDs, generated version.json, unrelated emulator apps or donor's published repository.
