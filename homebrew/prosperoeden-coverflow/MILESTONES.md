# ProsperoEden GBAXY Coverflow — Milestones and Handoff

**Updated:** 2026-10-27 UTC
**Source:** Yahballon/tmp_lap, feature/prosperoeden-gbaxy-coverflow
**Upstream:** Swordpdf/ProsperoEden @ a26ba788222465ec7fb94dc813958fc749734e07 (read-only permissions)
**PS5GBAX donor:** Yahballon/PS5GBAX @ 197c62e08cd709299785fa07c8c0addbb844c51d
**Title ID:** PPSA99008 (preserved), development variant 0.1.0
**Console test:** NOT PERFORMED. **Screenshot:** NOT CAPTURED.

## Evidence-based milestone matrix

| ID | Scope | Status |
| --- | --- | --- |
| M0 | Inspect current Eden native OpenGL launcher, donor Vulkan coverflow, upstream licensing and build | VERIFIED SOURCE |
| M1 | GBAXY pose math in coverflow.hpp, large centre, receding cases, fractional spring scroll | IMPLEMENTED, UNVERIFIED |
| M2 | Update library.cpp navigation, native OpenGL DrawList/GlBatch shader and game case reflection | IMPLEMENTED, UNVERIFIED |
| M3 | Preserve top-nav grid, game launch, source downloads, settings, save identity | SOURCE PRESERVED, DEVICE UNVERIFIED |
| M4 | Reproducible PS5 CI compile and package | WORKFLOW PREPARED, NOT VERIFIED |
| M5 | Verified actual Release and automatic TESTBUILDS entry | CONDITIONAL WORKFLOW PREPARED, NO RELEASE YET |
| M6 | Real PS5 startup, rendering, input, game launch and screenshot | NOT STARTED |

## Change log — 2026-10-27

- Inspected the donor PS5GBAX/ps5/frontend/fe_coverflow_layout.h and the ProsperoEden main-branch launcher renderer.
- Overlay now replaces the non-top-nav branch of headless/prosperoeden/pe/ui/library.cpp and keeps the current top-nav theme and game's details intact.
- Added headless/prosperoeden/pe/ui/coverflow.hpp with constant-time pose evaluation and bounded eleven-case UI rendering.
- Extended headless/prosperoeden/pe/gfx/draw_list.hpp, draw_list.cpp, and gl_batch.cpp with shader shape 9 for perspective-angled artwork; no changes made to the gameplay GPU backend.
- Added an overlay installer, pinned SHA manifest, CPU geometry test and native CI pipeline that clones upstream, applies sources, builds via original upstream CI, verifies ZIP, conditionally publishes a real test release and regenerates TESTBUILDS manifest via the existing generator.
- Full native CI run and hardware screenshot were not yet verified while authoring this handoff; publication MUST NOT be claimed without evidence.

## Regressions and risks

- The 2.5D OpenGL projection may need adjustments on actual PS5. Test large libraries, zero games, missing/wide artwork, rapid left/right input, L1/R1 navigation and remote progress.
- Verify cover cache, image aspect ratio, anti-aliased clipping and shader source compatibility with the PS5 OpenGL backend.
- Preserve original Title ID PPSA99008, saves, emulator, download service, features and packages; back up data for experimental test releases.
- Original Swordpdf/ProsperoEden has no write permission in the connected account, so work is distributed through the Yahballon/tmp_lap overlay.
- Existing CI/main checkout catalog logic accepts only a real PPSA99008.zip GitHub Release in the matching namespace; never fake version.json.

## NEXT ACTION FOR THE NEXT AGENT

**First:** inspect the Build ProsperoEden GBAXY Coverflow Actions run on feature/prosperoeden-gbaxy-coverflow. If it fails, read the first failing compiler/configuration line and fix the source or CI before resuming. If it builds, validate release asset/checksum and catalog entry, then test the actual PS5 launcher UI and game launch. Document console screenshots and log evidence before marking M6 verified.

**Do not change:** upstream emulator core, saved-game paths, PPSA99008, licensing, working top-nav theme or generated version.json manually.

## Verified host CI checkpoint — 2026-10-27

- Feature branch source commit: e7671479e61d3d560bb6a627605542ef22018c58.
- PR #275 merged to main: https://github.com/Yahballon/tmp_lap/pull/275 (merge commit 735b832a9ef5729044896ba8ec1c073f96fb79df).
- Host-only C++ coverflow geometry test **PASSED** in GitHub Actions step Verify native overlay C++ pose.
- Pinned upstream clone, application of all five changed UI/GPU files and source assertions **PASSED** in GitHub Actions step Clone pinned upstream and apply complete UI overlay.
- Evidence: https://github.com/Yahballon/tmp_lap/actions/runs/40561099313, job 122390014116.
- Native PS5 toolchain build and package steps were still in progress/unverified at this checkpoint. **No PS5 build success or Release is claimed.**
- The pull-request TESTBUILDS catalog validation also passed: https://github.com/Yahballon/tmp_lap/actions/runs/40561104802.

**NEXT ACTION UPDATE:** Inspect the native compiler and package-verification steps in run 40561099313. Publish status only after workflow confirms success, then validate the real release and auto-generated manifest. Real PS5 game launch and coverflow render remain untested.

## Native build failure and exact-source recovery — 2026-10-27

- [Run 40561099313](https://github.com/Yahballon/tmp_lap/actions/runs/40561099313) **FAILED** in `make deps` before compilation: `blackbearreloaded/ps5-opengl/releases/download/v1.0.2/ps5-opengl-sdk-1.0.2.tar.gz` returned HTTP 404 after three attempts. C++ pose and overlay application both passed.
- Found [darkxex/ps5-opengl v1.0.2](https://github.com/darkxex/ps5-opengl/releases/tag/v1.0.2), which exposes an archive of the **same filename and exact SHA-256** recorded in upstream `tools/deps.json`: `30d8e51dd062739b629346102be5932728ba714bc50b1bdcdf08306c8843d9a4`, file size 226310155 bytes.
- Added `tools/fix_opengl_sdk_mirror.py` with a strict original SHA and old URL guard. It substitutes the GitHub asset URL only in the temporary ProsperoEden build checkout. Upstream's SHA-256 verification still gates extracted inputs; *no checksum override is allowed*.
- Added `tests/test_sdk_mirror.py` to check idempotency, unrelated dependency preservation and rejection of altered hashes.
- Adjusted workflow to test the mirror patch and to release only on `main` after successful native compilation/ZIP verification. No Release or TESTBUILDS entry is implied by this patch until a succeeding native build confirms them.

**Next:** Inspect the rerun triggered by merging this fix to `main`: verify it passes `make deps` and proceeds to native compiler and ZIP validation; debug first failed job, and record only real results.
