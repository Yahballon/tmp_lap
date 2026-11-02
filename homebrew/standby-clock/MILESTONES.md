# Standby Clock — Milestones, Evidence & Agent Handoff

**Project:** Yahballon/tmp_lap/homebrew/standby-clock
**Last updated:** 2026-11-02 UTC
**Branch:** main (PR #297 merged)
**Version / Title ID:** 0.1.1 / PPSA99876
**Verified Release:** [standby-clock-v0.1.1](https://github.com/Yahballon/tmp_lap/releases/tag/standby-clock-v0.1.1) — PPSA99876.zip (9,188,188 bytes)
**Hardware target:** Jailbroken PS5; runtime compatibility NOT VERIFIED

> CURRENT STATE: Standby Clock v0.1.1 compiled and packaged by main CI run 41423379609; immutable release published and generated 34-app TESTBUILD catalog includes v0.1.1 PPSA99876. New weather/OLED behavior still needs console testing.

## Milestone matrix

| ID | Acceptance criteria | Evidence | Status |
| --- | --- | --- | --- |
| M0 | Inspect reference apps/build and collision-check title | tmp_lap native source and app metadata | Completed |
| M1 | Clock, analog hands, dates and week strip | native/main.cpp and clock_logic.hpp | Implemented / unverified |
| M2 | Alarm, snooze, settings persistence, theme/brightness | Native source | Implemented / unverified |
| M3 | Portable unit tests | run 41417779557 | CI VERIFIED |
| M4 | Native PS5 installer | run 41417779557, ZIP artifact Standby-Clock-v0.1.0-PPSA99876 (6,608,501 bytes) | CI VERIFIED |
| M5 | Runtime PS5 screenshot and controller test | Console test needed | NOT STARTED |
| M6 | TESTBUILD catalog auto-discovery | Main `version.json` schema 2, 34 entries including PPSA99876 | VERIFIED IN CATALOG |

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

### 2026-11-02 — v0.1.1 local weather and OLED guard
- Added `weather_model.hpp`, `weather_client.cpp`, `weather_test.cpp`: asynchronous verified HTTPS to ipapi.co and Open-Meteo, local cache/offline states, geolocation without GPS, no fake readings
- Replaced old Homebrew/READY footer block with temperature, approximate location and conditions
- Fixed the 12-hour AM/PM suffix under the giant clock by putting it next to the date instead
- OLED guard: full-screen transform pixel shifts, idle/night dim, 20-second periodic blank display, options to disable
- Backward-compatible settings v1 migration to v2; ContentVersion 01.000.002, release tag standby-clock-v0.1.1
- Status: CI COMPILATION / PACKAGE / RELEASE VERIFIED; HARDWARE WEATHER AND OLED QA PENDING
- PR #297 merged in main commit `3a1b9d0bc96b4bea591446a006d5ed8aeac3a449`; build 41423379609 SUCCESS, release asset PPSA99876.zip size 9,188,188 bytes, catalog lists v0.1.1.
- NEXT: on-console TESTBUILD test. Confirm IP geolocation accurate enough for ISP, HTTPS succeeds, weather refresh and cached/offline states, no AM under numerals, OLED shift/idle/night dim/rest.


### 2026-11-02 — Main release and TESTBUILD catalog verified
- PR #296 merged to main at commit `98e49b2a5fb3bd9d9e4861fbab45682172c3560b`.
- Main-branch GitHub Actions [41420993845](https://github.com/Yahballon/tmp_lap/actions/runs/41420993845) SUCCESS: native compile, installer ZIP verification, Actions artifact and GitHub Release publish all passed.
- Immutable release [standby-clock-v0.1.0](https://github.com/Yahballon/tmp_lap/releases/tag/standby-clock-v0.1.0) contains uploaded `PPSA99876.zip` (6,708,872 bytes).
- Catalog workflow [41421237141](https://github.com/Yahballon/tmp_lap/actions/runs/41421237141) SUCCESS; generated `version.json` schema 2 has 34 apps and valid Standby Clock entry (`PPSA99876`).
- PS5 hardware test NOT PERFORMED. User should refresh catalog in TESTBUILD and test launch, clock rendering, controls, alarm, and settings.


### 2026-11-02 — Native PS5 CI success, before merge
- GitHub Actions [run 41417779557](https://github.com/Yahballon/tmp_lap/actions/runs/41417779557) completed SUCCESS with host tests, SDK preparation, native PS5 compilation, package checks and artifact upload.
- Actual package: Actions artifact `Standby-Clock-v0.1.0-PPSA99876`, 6,608,501 compressed bytes. This is an Actions artifact, NOT yet a catalog-valid GitHub Release.
- PS5 hardware: NOT TESTED. Release/tag/catalog still not published at this checkpoint.
- Next: merge PR #296, verify main-branch build and published `PPSA99876.zip`, check generated `version.json` includes `PPSA99876`.


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
- No proven hardware screenshot or evidence of PS5 launch yet. Package structure and native executable were validated in CI, not on console.
- App cannot trigger a system wake alarm while PS5 sleeps
- Device clock/local timezone behavior unverified

## Roadmap — prioritized acceptance tests
### P0 — Immediate
- [x] Run pull-request CI, confirm compile/package success (41417779557)
### P1 — TESTBUILD
- [x] Merge, release `PPSA99876.zip`, verify generated catalog contains Standby Clock
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
**Do this first:** Install Standby Clock v0.1.1 via the TESTBUILD catalog and verify the weather panel, AM placement, and OLED settings on actual console. Capture screenshot and `/data/StandbyClock/clock.log`.
**Starting point:** `Yahballon/tmp_lap`, `main`, merged PR #297, release tag `standby-clock-v0.1.1`.
**Expected success evidence:** PS5 screenshot of rendered digital/analog clock and log from `/data/StandbyClock/clock.log`; CI artifact and package already verified.
**If it fails:** Supply screen/log and inspect runtime font loading, coordinates and clock API; patch only standby-clock app/build.
**Do not change:** Existing app Title IDs, manually-generated `version.json`, or unrelated homebrew.
