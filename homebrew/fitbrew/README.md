<div align="center">

# FITBREW

**Your body. Your goals. Zero equipment. Zero subscriptions.**

Native PlayStation 5 Homebrew · **by YAHBALLON** · v0.1.3

</div>

## Fitness training, not a fitness paywall

FITBREW is an entirely offline, controller-first fitness application for the PS5, designed around bodyweight-only exercise. No machine, dumbbell, band, subscription, sign-in, camera, smartwatch or personal data upload is needed.

> **PS5 test feedback:** A user reports that the core app appears to work on PS5 but exercise pictures were missing in v0.1.0. v0.1.3 addresses this with raw GPU textures and additional logs; the fix still needs an on-console retest. The bundled six images are generated illustrations until original photo files are provided. The uploaded reference design is not a hardware screenshot.

## Features

- **Dashboard:** dark professional, lime-accented TV interface based on the supplied FITBREW reference, interactive cards, local history summary and seven-day minutes graph.
- **12 guided workouts:** Full Body Starter, Low Impact Cardio, Core Control, Full Body HIIT, Leg Day, Chest & Core, Morning Energy, No-Jump Sweat, Bodyweight Burn, Posture Reset, Glutes & Legs and Quick 10.
- **36 exercises:** bodyweight movements with form cues and a less demanding alternative; *no exercise gear*.
- **Session timer:** separate work/rest phases, pause/resume, skip and previous movement; visual progress and next-exercise information.
- **Local progress:** completed sessions, cumulative active time and day streak, persisted in `/data/FITBREW/progress.ini`; daily buckets use UTC.
- **Offline-first:** no phone pairing, cloud login, telemetry or fitness-device sensor assumptions.
- **No invented metrics:** calories, heart rate and steps are **not** displayed as measured numbers, since no verified sensor source is available.

### Workout library

| Group | Plans | Focus |
| --- | --- | --- |
| Beginner | Starter, Low Impact Cardio, Core Control, Morning Energy, Glutes & Legs, Quick 10, Posture Reset | Foundational movement, moderate pace |
| Intermediate | Full Body HIIT, Leg Day, Chest & Core, No-Jump Sweat | Progressive bodyweight sessions |
| Advanced | Bodyweight Burn | Fast-paced intervals with higher-impact movements |

Each plan has warm-up / recovery elements where included in its sequence. Follow each exercise's own on-screen cues. If a movement causes pain, stop and seek appropriate professional advice. Intensity and individual suitability vary.

## DualSense controls

| Location | Input | Result |
| --- | --- | --- |
| Menus | D-pad or left stick | Navigate focus |
| Menus | X | Details / select |
| Menus | Triangle | Quick-start a highlighted workout |
| Menus | L1 / R1 | Switch sections |
| Menus | Circle | Back / exit prompt |
| In workout | X | Pause / resume |
| In workout | Square | Advance to next phase |
| In workout | Triangle | Previous exercise or leave rest |
| In workout | L1 | Leave session without recording completion |
| In workout | Circle / Options | Pause / resume |

## Build and installation

This application lives at `homebrew/fitbrew/` in [Yahballon/tmp_lap](https://github.com/Yahballon/tmp_lap). GitHub Actions builds the PS5 native binary with the [PS5-DevBridge](https://github.com/Yahballon/PS5-DevBridge) template and [Yahballon/ps5-homebrew-ui](https://github.com/Yahballon/ps5-homebrew-ui).

- Workflow: [build-fitbrew.yml](../../.github/workflows/build-fitbrew.yml)
- Native entry point: [native/main.cpp](native/main.cpp)
- Workout definitions/state machine: [native/workout.hpp](native/workout.hpp)
- Test: `g++ -std=c++20 -O2 -Wall -Wextra -Werror -pedantic homebrew/fitbrew/tests/workout_test.cpp -o /tmp/fitbrew-test && /tmp/fitbrew-test`
- Art staging: `python3 homebrew/fitbrew/assets/generate_assets.py app`
- Expected build artifact after successful CI: `PPSA99790.zip`, containing `PPSA99790/eboot.bin`, `sce_sys/icon0.png`, required runtime libraries, font and pose textures.
- Once the release workflow succeeds on `main`, TESTBUILDS' standard `version.json` generator discovers its release asset automatically. **Do not manually edit `version.json`.**
- Application logs: `/data/FITBREW/logs/fitbrew.log`.

**State of support:** PS5 target firmware, OpenGL runtime, physical controller response, image load and real frame timing must still be checked on-console. No release/TESTBUILD is claimed before the build succeeds.

## Photography and image troubleshooting

FITBREW v0.1.3 bundles twelve **original AI-generated FITBREW fitness images** in the native installable PS5 ZIP. The original generated source images are committed to GitHub, resized and converted to RGBA8 during the build, then bundled with JPEG fallbacks. **No USB photo pack or console network connection is required.** Build failures block publication rather than silently generating stick figures.

Each of the 12 curated plans has a different photo; exercise breakdown thumbnails intentionally reuse corresponding movement categories, not exact tutorial still frames. True technique animation for each of 36 exercises is a later feature.

Source-image mapping: [PHOTO_CREDITS.md](assets/PHOTO_CREDITS.md). To diagnose missing GPU textures, inspect `/data/FITBREW/logs/fitbrew.log` for `ART_RGBA` and `ART_READY 12/12 textures`. Nothing in the v0.1.3 release claims to have been hardware-verified until retested.
## Privacy and safety

Training history is saved on the PS5 only. The app does not measure heart rate, calories, steps, oxygen saturation or health conditions. Exercise prescriptions are general, not individual medical advice. Start gently; avoid movements unsuitable for your circumstances.

## Development status and next steps

See [MILESTONES.md](MILESTONES.md) for build evidence, open issues, verified hardware behavior and next-agent handoff. No device screenshot has been claimed.

## Credits

**YAHBALLON** — concept, source and PS5 homebrew integration. PS5-DevBridge and ps5-homebrew-ui are upstream development dependencies; retain their respective licenses. FITBREW code is MIT-licensed where marked.
