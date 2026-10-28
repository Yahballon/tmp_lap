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

## Additional upstream Git sources recovery — 2026-10-27

GitHub REST also returned 404 for both original `blackbearreloaded/ps5-native-app-boilerplate` and `blackbearreloaded/PS5-Lapy-JB-Daemon` repositories. ProsperoEden pins exact Git objects, not floating branches. Their exact commits were verified accessible in:

- `drakmor/ps5-native-app-boilerplate@dd44bbdc75437332ed22e3ba95126733419ef25a`
- `Phononenom/PS5-Lapy-JB-Daemon-bbr@57fcc19b7f57e46c823d6f2a6c6233ebf3288079`
- Original OpenGL SDK `darkxex/ps5-opengl@v1.0.2` asset digest `30d8e51dd062739b629346102be5932728ba714bc50b1bdcdf08306c8843d9a4`

The mirror-switch script now guards **all three original URL + exact digest/commit pairs**, fails atomically if the upstream metadata changes, and does not weaken the original archive SHA checks or git commit identity checks. The SDK and git repos are still trusted/validated only under their original upstream exact pins. A new build is required to verify fetching the mirror at runtime; presence of GitHub commit objects alone is not proof of completed native compilation.

**Next:** Inspect the latest `main` Actions run. Pass criteria are make deps (all dependencies fetched at exact pins), complete native eboot compile, ZIP integrity, GitHub Release asset and generated TESTBUILDS entry. Hardware check and actual screenshot remain outstanding.

## UI-kit mirror restored — 2026-10-27

- Native [run 40563453423](https://github.com/Yahballon/tmp_lap/actions/runs/40563453423) failed during `make deps` downloading ten per-file SHA-256-pinned assets at `blackbearreloaded/ps5-homebrew-ui@aed286b81cdf70a814d3059f54633ba34844e46e`, HTTP 404 for `src/ui/theme.cpp`. The previous OpenGL SDK source mirror worked in this run.
- Verified that the exact source commit `aed286b81cdf70a814d3059f54633ba34844e46e` exists in `Yahballon/ps5-homebrew-ui`. Updated the CI-only URL translation to point all ten original files to this exact commit in the user's fork. Each individual file's expected SHA-256 is pinned unchanged and still verified by ProsperoEden's `tools/deps.py`.
- Expanded regression tests: all ten names, SHA-256 pins, idempotency and fail-closed behavior on any unexpected change. This patch does not alter Eden gameplay or package identity. A CI compile and device validation remain required.

**Next action:** review the new main branch run, confirm `make deps` completes, then tackle first native compiler/linker issue. Do not claim Release or TESTBUILDS availability without an actual verified asset.

## v0.1.1 side cover framing repair — 2026-10-28

- User provided actual PS5 photo of v0.1.0 coverflow: selected centre case looked correct, but left/right side artwork was visibly misaligned with the underlying large square frames, with edges clipping on the right side.
- Root cause: side cases have a perspective-compressed texture quad but unwarped `bordered_rect` and shadow covering the full square case. Side positions also advanced 138 px per case, pushing covers beyond the library viewport.
- Fixed: only the central card draws the full flat highlight frame; side covers draw their artwork in the same projected geometry, with a balanced 3px inset. Changed yaw 0.82 -> 0.62, first-side spread 335 -> 305, tail spread 138 -> 68 with bounded tail, side scale 0.94 -> 0.84 to keep neighbours within the visible panel. The centre case remains 384px, retaining the successful v0.1.0 appearance.
- Original cover images stay square. A fixed portrait aspect of 0.7, previously suggested before inspecting source, would be incorrect for the pictured Switch game artwork and is intentionally not used.
- Floor reflections track the projected inner artwork. Focus ring appears only on the dominant centre case, preventing large flat borders on angled sides. Missing covers and remote download progress retain the original fallbacks.
- Updated app.json and build workflow's immutable GitHub Release tag to `prosperoeden-coverflow-v0.1.1` so TESTBUILDS auto-generator can pick up the new build rather than silently reusing v0.1.0.
- Added host regression for square case geometry and visible left/right bounds. A passing GitHub native build and PS5 screenshot of the new version are still required for validation.

**NEXT AGENT:** inspect native workflow for v0.1.1, confirm CI ZIP and release; user should install with TESTBUILD and take PS5 screenshot. Do not assert the real PS5 visual issue is definitively fixed before their test.
