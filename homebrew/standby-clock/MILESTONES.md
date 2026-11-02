# Standby Clock — Milestones, Evidence & Agent Handoff

**Project:** Yahballon/tmp_lap/homebrew/standby-clock
**Last updated:** 2026-11-02 UTC
**Branch:** feature/standby-clock-v0.1.0
**Version / Title ID:** 0.1.0 / PPSA99876
**Verified Release:** NONE
**Hardware target:** Jailbroken PS5; runtime compatibility NOT VERIFIED

> CURRENT STATE: Native C++ realtime clock and controller settings have been authored in a GitHub feature branch. Native cross-compile, ZIP packaging and actual console tests have NOT been verified. Merging to main is required to run the main-branch release workflow.

## Milestone matrix

| ID | Acceptance criteria | Evidence | Status |
| --- | --- | --- | --- |
| M0 | Inspect reference apps/build and collision-check title | tmp_lap native source and app metadata | Completed |
| M1 | Clock, analog hands, dates and week strip | native/main.cpp and clock_logic.hpp | Implemented / unverified |
| M2 | Alarm, snooze, settings persistence, theme/brightness | Native source | Implemented / unverified |
| M3 | Portable unit tests | tests/clock_test.cpp | Implemented; CI pending |
| M4 | Native PS5 installer | build-standby-clock.yml | Build NOT RUN |
| M5 | Runtime PS5 screenshot and controller test | Console test needed | NOT STARTED |
| M6 | TESTBUILD catalog auto-discovery | Existing release parser and workflow_run trigger | Integration prepared; not verified |

## Architecture and anchors

- Runtime: ps5-homebrew-ui via Yahballon/PS5-DevBridge simple-calc; OpenGL renderer, system clock, Pad
- Native source: `homebrew/standby-clock/native/main.cpp`
- Calendar logic: `homebrew/standby-clock/native/clock_logic.hpp`
- Build: `.github/workflows/build-standby-clock.yml`
- Metadata: `homebrew/standby-clock/app.json`
- Generated install root: `PPSA99876/`, with icon at `sce_sys/icon0.png`
- Settings: `/data/StandbyClock/settings.ini`; log: `/data/StandbyClock/clock.log`
- TESTBUILDS: `tools/update-version.py` generates `version.json` from valid published GitHub Release assets

## Change log — latest first

### 2026-11-02 — Initial CI failure, SDK archive manifest compatibility fix
- Pull request #296, first Actions run 41417627816: host clock tests PASS; framework staging and generated icon PASS
- Build dependencies FAILED before compilation: `unexpected ps5-opengl SDK manifest: 0d432edc9d648d6e0cccb4d5eb3db8484f10ba4b075b5ccf23edda3251b66360`
- Updated SDK mirror compatibility handling to match working 4 WINS and Drink Roulette workflows; archive SHA-256 remains pinned. New CI outcome pending
- Next: inspect rerun for native compiler/linker errors, check zipped installer and only then merge


### 2026-11-02 — Initial native implementation (feature branch)
- Authored native clock renderer, digital/analog time, calendar, alarm and snooze, themes and settings
- Added portable tests for leap years, week crossing, alarm and time format
- Added asset generation, installable ZIP workflow, README, release notes
- Validation: code committed. CI compile and console execution NOT yet observed
- Risk: font size and digital character positions require genuine 1920×1080 PS5 screenshot; confirm timezone and persistent storage on user hardware
- Alarm is foreground-only visual notification (no wake from suspend or alarm sound)

## Working features — do not regress
- No hardware-tested baseline exists yet

## Known bugs / limitations
- No proven hardware screenshot, no verified install ZIP, no evidence of PS5 launch yet
- App cannot trigger a system wake alarm while PS5 sleeps
- Device clock/local timezone behavior unverified

## Roadmap — prioritized acceptance tests
### P0 — Immediate
- [ ] Run pull-request CI and inspect actual compiler/linker output; repair errors before merge
### P1 — TESTBUILD
- [ ] Merge a passing build, verify release tag and `PPSA99876.zip`, confirm catalog index includes Standby Clock
### P2 — Console
- [ ] Run from TESTBUILD, record the on-console UI screenshot, controller behavior, date/time correctness, alarm and exit

## Build / install / test playbook
1. Run `g++ -std=c++20 -Wall -Wextra -Werror -pedantic homebrew/standby-clock/tests/clock_test.cpp -o /tmp/clock-test && /tmp/clock-test`
2. Verify `.github/workflows/build-standby-clock.yml` has a green compile/package job
3. On main-branch success, confirm the real Release `standby-clock-v0.1.0` has nonempty `PPSA99876.zip`
4. Let the existing generator update `version.json`; never modify it by hand
5. Install via TESTBUILD and verify Home screen, settings, alarm, persistence and Circle exit
6. Compare actual 1920x1080 screenshot with user's red-on-black visual reference

## NEXT ACTION FOR THE NEXT AGENT
**Do this first:** Review the running pull request check for `Build Standby Clock PS5`, fix the earliest native compile failure (if any), then validate ZIP structure.
**Starting point:** `Yahballon/tmp_lap`, `feature/standby-clock-v0.1.0`.
**Expected success evidence:** Green Actions job with `PPSA99876.zip` and host tests passing.
**If it fails:** Use full job step logs; patch only standby-clock app/build unless shared SDK regression is proven.
**Do not change:** Existing app Title IDs, manually-generated `version.json`, or unrelated homebrew.
