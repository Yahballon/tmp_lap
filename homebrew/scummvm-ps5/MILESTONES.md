# ScummVM PS5 — Milestones, Evidence & Agent Handoff

**Updated:** 2026-11-01 UTC
**Source:** Yahballon/tmp_lap, feature/scummvm-ps5-core
**Upstream frontend:** mihawk-99/PS5_RetroArch @ 7268510b12fe7461342e0cc4497c87a0910c1055
**Upstream engine:** mihawk-99/PS5_ScummVM @ 967fd9757170b14c28590837186b31cc7d5cc46c
**Version / candidate Title ID:** v0.1.0 / PPSA99053, no global reservation
**Release / TESTBUILD:** NONE; generated version.json not touched; CI run 41289470054 FAILED at SDK bootstrap
**Console test:** NOT PERFORMED; **Screenshot:** NOT CAPTURED

> **CURRENT STATE:** ScummVM PS5 upstream core and PS5 RetroArch sources inspected. A dedicated, ScummVM-only build overlay and CI are prepared for actual native build. No dedicated-title PS5 execution has been observed.

## Milestone matrix

| ID | Goal | Status | Evidence |
| --- | --- | --- | --- |
| M0 | Identify usable PS5 ScummVM core | Source verified | PS5_ScummVM fork and PS5_RetroArch's build-scummvm.sh pinned commits |
| M1 | Dedicated unique-ID ScummVM-only frontend and icon | IMPLEMENTED / UNVERIFIED | scripts/prepare.py plus source artwork; CI pending |
| M2 | Genuine PS5 compilation and signed ZIP | FAILED AT SDK BOOTSTRAP | Run [41289470054](https://github.com/Yahballon/tmp_lap/actions/runs/41289470054); SDK fork checkout fix committed on feature branch |
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
