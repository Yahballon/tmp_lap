# FITBREW — Milestones, Evidence & Agent Handoff

**Project:** `Yahballon/tmp_lap/homebrew/fitbrew`
**Last updated:** 2026-10-26 (UTC; exact minute not recorded)
**Active branch:** `main`; v0.1.1 image fix merged via PR #272 (squash commit `4ab6d0ab7d4fac48f81166d5aa78a8cf5c785e13`)
**Current version / Title ID:** v0.1.1 released / PPSA99790 (unchanged) (searched monorepo and PHIP-0001 for collision; no matching source-index result; global uniqueness NOT guaranteed)
**Latest verified Release:** [fitbrew-v0.1.1](https://github.com/Yahballon/tmp_lap/releases/tag/fitbrew-v0.1.1), asset `PPSA99790.zip` (6,886,686 bytes); main build run 40322034573 SUCCESS. TESTBUILDS catalog update run 40322320329 SUCCESS, current `version.json` shows FITBREW v0.1.1.
**Hardware target:** user's PS5 firmware 5.10. On 2026-10-26 user reports most functionality seems operational except missing photos in v0.1.0. No logs/screenshots or v0.1.1 retest supplied.

> **CURRENT STATE:** FITBREW v0.1.1 is published and available from the updated TESTBUILDS catalog. Six raw 512×512 textures and native packaging passed host/PS5 CI. The PS5 user reported the v0.1.0 app generally worked but images did not; v0.1.1's runtime image fix still needs hardware confirmation and an `ART_READY 6/6 textures` log.

## Status legend
- VERIFIED ON PS5: actual console evidence
- CI / BUILD VERIFIED: compiler / package evidence
- IMPLEMENTED / UNVERIFIED: source present without confirmed execution
- BLOCKED: known failure
- NOT STARTED: not yet implemented

## Milestone Matrix

| ID | Goal | Current status | Evidence |
| --- | --- | --- | --- |
| M0 | Existing repository/build stack researched | DONE | `4wins` native reference and PS5-DevBridge |
| M1 | 36 bodyweight-only exercises and 12 plans | IMPLEMENTED / UNVERIFIED | `native/workout.hpp` |
| M2 | Timer, rest, pause, skip, local progression model | IMPLEMENTED / UNVERIFIED | `native/workout.hpp` |
| M3 | Native dashboard/library/details/session/progress/settings | IMPLEMENTED / UNVERIFIED | `native/main.cpp` |
| M4 | Six RGBA8 image resources and icon stage | CI / BUILD VERIFIED | 40322034573 passed host art tests and asset ZIP checks |
| M5 | Host exercise test | CI / BUILD VERIFIED v0.1.1 | Main job 40322034573, both workout and art tests passed |
| M6 | Native PS5 executable and installer ZIP | CI / BUILD VERIFIED v0.1.1 | Main job 40322034573, release `fitbrew-v0.1.1` |
| M7 | PS5 controller / OpenGL hardware run | PARTIAL USER REPORT v0.1.0 | User reports functions seem to work except images; full functionality not confirmed |
| M8 | Actual PS5 screenshot `screenshots/1.png` | NOT CAPTURED | console needed |
| M9 | User-approved final original hero and guided exercise imagery | NOT INTEGRATED | generated AI collage local to task, missing from GitHub |

## Architecture and Technical Anchors

- Native C++20 with Yahballon `ps5-homebrew-ui`, native `Display`, `Renderer`, `Pad`, `InputTracker`.
- `app.json` declares `PPSA99790`, `UP9000-PPSA99790_00-FITBREWHOMEBREW00`, `fitbrew-v0.1.0`.
- Static exercise/plan model: `native/workout.hpp`. Screen rendering, storage, app loop: `native/main.cpp`.
- Local progress: `/data/FITBREW/progress.ini`, logs: `/data/FITBREW/logs/fitbrew.log`. Progress stored with temp-write then atomic rename.
- CI: `.github/workflows/build-fitbrew.yml`; `tools/update-version.py` discovers only **successful real Releases**, not PRs.
- Build setup copied from `build-4wins.yml`. Needs first actual CI invocation and compiler errors fixed rather than assumed working.

## Change Log — Latest First

### 2026-10-26 — v0.1.1 release and TESTBUILDS verification
- PR #272 merged; main Actions run 40322034573 succeeded, including host art test, PS5 compiler, ZIP verification of all six `.rgba` and `.jpg` assets and uploaded installer.
- Published immutable release `fitbrew-v0.1.1`, `PPSA99790.zip` = 6,886,686 bytes.
- Catalog workflow 40322320329 passed; `version.json` automatically updated to `0.1.1`, title ID PPSA99790, matching tag and ZIP URL (30 entries).
- Photo pack ZIP created outside repository: `FITBREW-Photo-Pack-v0.1.1.zip`, containing six original AI picture overrides. It is not embedded in GitHub Release. Pack can be copied to root of USB and unzipped as `FITBREW/assets/pose_*.rgba`.
- PS5 v0.1.1 image visibility still not hardware-confirmed. User should check console image and `ART_READY 6/6 textures` in logs.
\n\n### 2026-10-26 — v0.1.1 missing image workaround (unverified)\n- User tested v0.1.0 in TESTBUILDS and reports images absent; other functions apparently work. No console log was supplied.\n- Working hypothesis: JPEG texture upload/decoding or lookup path on PS5; cannot distinguish without logs.\n- Native loader now attempts 512×512 raw RGBA8 GPU uploads first and reports bytes and resulting texture ID. JPEG fallback remains for compatibility.\n- Asset generator writes six `.rgba` images alongside `.jpg`; added host validation and ZIP asset-size checks.\n- Optional original photo pack is loaded from `/data/FITBREW/assets` or USB 0–3 before packaged fallback; photos have not been embedded in the GitHub source.\n- No v0.1.1 CI or PS5 result claimed at the time of this checkpoint.\n

### 2026-10-26 — Immutable release workflow guard (pending CI)\n- Initial release is immutable. Subsequent documentation edits triggered expensive builds and failed at the existing release tag.\n- Restricted FITBREW build workflow triggers to native source, assets, tests, app.json and workflow changes.\n- Existing-release condition fetches tag and checks native/assets/app.json bytes are unchanged before safely skipping publish; changed sources require a new version/tag.\n- Verify `fix/fitbrew-release-rebuild-guard` CI and merge.\n\n### 2026-10-26 — Main release verified and catalog trigger fix\n- PR #269 merged (squash commit 45ad7848c6fc1317b99906bb5b6598d366a9e0bb).\n- Main Actions run 40317621212 succeeded and published release \`fitbrew-v0.1.0\`; verified the uploaded 6,823,203-byte \`PPSA99790.zip\`.\n- Catalog `tools/test-catalog-triggers.py` requires workflow_run subscription to all publishers. FITBREW was absent from `.github/workflows/update-version-catalog.yml`, so add `Build FITBREW PS5` and regression assertion.\n- Release is downloadable, but TESTBUILDS listing remains unverified until refresh succeeds.\n\n### 2026-10-26 — CI build verified\n- PR CI run 40317262158 passed, including 36-exercise host tests, native PS5 OpenGL SDK build, actual package binary checks and uploaded installable ZIP artifact.\n- Test on physical PS5 firmware 5.10 is still NOT PERFORMED.\n- Next: merge PR if authorized, let main run publish the Release, verify TESTBUILDS catalog after generator finishes.\n\n### 2026-10-26 — First CI failure and metadata fix\n- Actions run `40317053412`, job `121552171539`: host workout tests, art staging, OpenGL dependencies passed. Native package stage failed with `param.json contentId must be valid and contain titleId`.\n- Root cause: app.json contentId suffix length was 17, required 16. Updated to `UP9000-PPSA99790_00-FITBREWHOMEBREW0`; pending next CI run for proof.\n- The first CI job ran before the latest main.cpp string fix was pushed. Inspect new CI.\n\n### 2026-10-26 — First native implementation (feature branch)
- User requested 1:1 FITBREW-inspired PS5 app with equipment-free programs, premium imagery, no follow-up questions and TESTBUILDS.
- Added app metadata, 36 exercises, 12 programs and initial simulation checks.
- Added native live rendered UI with dark/lime styling and per-exercise timers.
- Added image stage with optional `assets/pose_0.jpg..pose_5.jpg` and generated fallback illustrations.
- Added build workflow and packaged-art validation; GitHub Actions results **not yet inspected**.
- Device tests, source-accurate screenshot and final AI photo assets remain open.

## Working Features — Do Not Regress

User reports most FITBREW v0.1.0 navigation and workouts work on PS5; treat this as preliminary user feedback, not proof of every feature. Preserve Title ID, settings path, exercises, plans and input behavior.

## Known Bugs, Blockers and Limitations

- Unverified SDK compatibility; compilation may expose issues in `main.cpp`, UI API or project paths.
- No hardware logs; display scaling/input exact behavior unknown.
- Screen focus and timer timing need regression validation, especially Circle/Options and L1 during sessions.
- Settings preference currently changes the stored level only; it does not auto-adjust difficulty or plan sequences. Do not advertise adaptive workouts.
- `History::day_now()` uses UTC; daily summaries and streak rollover may not reflect local midnight.
- No real measurement inputs; no measured heart rate, step count or calories.
- AI fitness-photo montage exists only as local generated output; CI uses illustrated fallback until photo sources are added.
- No video or multi-frame technique demonstration yet; each exercise has one representative image and text cues, not a verified technique video.
- Screenshot reference is a mockup. `screenshots/1.png` must be a true native capture, not the mockup.

## Roadmap — Prioritized Acceptance Tests

### P0 — Immediate next step
- [ ] Open PR and inspect FITBREW CI run, fix compilation/runtime staging errors, verify genuine `PPSA99790.zip` contents.

### P1 — Real PS5 execution
- [ ] Load in TESTBUILDS when valid Release exists, confirm launch on firmware 5.10.
- [ ] Test Home → Workout → Detail → Session → Completion, repeat, pause, navigate, reset.
- [ ] Validate persisted progress across app restart; verify path permissions.
- [ ] Capture actual UI as `screenshots/1.png` and inspect layout against screenshot.

### P2 — Premium quality improvements
- [ ] Check in generated original photo atlas cropped to six (or more) native poses and professional AI icon; update package + README.
- [ ] Add exercise-specific sequential images or short safe animations, warm-up/cool-down options and accessibility.
- [ ] Add sound cues, weekly programming and difficulty-based exercise substitutions after hardware validation.

## Build / Install / Test Playbook

1. `g++ -std=c++20 -O2 -Wall -Wextra -Werror -pedantic homebrew/fitbrew/tests/workout_test.cpp -o /tmp/fitbrew-test && /tmp/fitbrew-test`
2. Build via `.github/workflows/build-fitbrew.yml`, inspect both CI step output and native file magic.
3. After successful main build, inspect Release `fitbrew-v0.1.0` and ZIP `PPSA99790.zip`. TESTBUILDS catalog is regenerated by existing Action — do not hand-write `version.json`.
4. On PS5, start app, confirm main menu, D-pad navigation, X selection, session counter, square skip, pause, repeat, load/save. Check `/data/FITBREW/logs/fitbrew.log`.
5. Keep test artifacts, actual screenshots and logs only if verified.

## NEXT ACTION FOR THE NEXT AGENT

**Do this first:** merge CI-green PR if appropriate, verify main build creates real release ZIP `PPSA99790.zip`, then verify generated TESTBUILDS catalog. Fix any compiler errors before merging and releasing.  
**Starting point:** `Yahballon/tmp_lap`, branch `main`, release `fitbrew-v0.1.1`, `homebrew/fitbrew/`, `.github/workflows/build-fitbrew.yml`.  
**Expected success evidence:** passing host test, actual PS5 compile, verified ZIP with real `eboot.bin`, icon, six JPEGs, fonts, libc.  
**If it fails:** inspect failing job and stage artifacts; compare with working `build-4wins.yml` pattern.  
**Do not change:** `tmp_lap/version.json` manually, other apps or existing game Title IDs.  
**Requires hardware:** real console rendering, timers and capture on PS5 firmware 5.10.
