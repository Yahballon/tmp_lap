# FITBREW — Milestones, Evidence & Agent Handoff

**Project:** `Yahballon/tmp_lap/homebrew/fitbrew`
**Last updated:** 2026-10-26 (UTC; exact minute not recorded)
**Active branch:** `feature/fitbrew-native-fitness` (working draft; inspect branch head)
**Current version / Title ID:** v0.1.0 / PPSA99790 (searched monorepo and PHIP-0001 for collision; no matching source-index result; global uniqueness NOT guaranteed)
**Latest verified release / TESTBUILD:** NONE
**Hardware target:** user's PS5 firmware 5.10; not tested for this app

> **CURRENT STATE:** New workout engine, native UI implementation, asset-stage script and GitHub Actions workflow are present on a feature branch. No successful CI compilation, installable ZIP or hardware run is verified. Final original AI photo atlas was generated in the agent session but **not committed as runtime assets**; deterministic artwork fallback exists in generator.

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
| M4 | Six exercise image resources and icon stage | IMPLEMENTED / UNVERIFIED | `assets/generate_assets.py`; illustrated fallback |
| M5 | Host exercise test | NOT RUN | `tests/workout_test.cpp` |
| M6 | Native PS5 executable and installer ZIP | NOT VERIFIED | `.github/workflows/build-fitbrew.yml` |
| M7 | PS5 controller / OpenGL hardware run | NOT STARTED | console needed |
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

### 2026-10-26 — First native implementation (feature branch)
- User requested 1:1 FITBREW-inspired PS5 app with equipment-free programs, premium imagery, no follow-up questions and TESTBUILDS.
- Added app metadata, 36 exercises, 12 programs and initial simulation checks.
- Added native live rendered UI with dark/lime styling and per-exercise timers.
- Added image stage with optional `assets/pose_0.jpg..pose_5.jpg` and generated fallback illustrations.
- Added build workflow and packaged-art validation; GitHub Actions results **not yet inspected**.
- Device tests, source-accurate screenshot and final AI photo assets remain open.

## Working Features — Do Not Regress

None verified on hardware for FITBREW v0.1.0 yet. Existing other apps are unaffected; only FITBREW and its new workflow were edited.

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

**Do this first:** inspect FITBREW PR GitHub Actions and run host tests. Fix any compiler errors before merging and releasing.  
**Starting point:** `Yahballon/tmp_lap`, branch `feature/fitbrew-native-fitness`, `homebrew/fitbrew/`, `.github/workflows/build-fitbrew.yml`.  
**Expected success evidence:** passing host test, actual PS5 compile, verified ZIP with real `eboot.bin`, icon, six JPEGs, fonts, libc.  
**If it fails:** inspect failing job and stage artifacts; compare with working `build-4wins.yml` pattern.  
**Do not change:** `tmp_lap/version.json` manually, other apps or existing game Title IDs.  
**Requires hardware:** real console rendering, timers and capture on PS5 firmware 5.10.
