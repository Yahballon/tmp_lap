# Drink Roulette by YAHBALLON — v0.1.3

Native PS5 homebrew party game with 1920×1080 OpenGL presentation, neon drink-roulette lobby, an animated green glass bottle sprite, pass-and-play controller controls, and a fully offline set of **433 unique challenges**. The provided lobby design is used as the background; foreground roulette, challenge cards, and menus are rendered live.

**PS5 Title ID:** `PPSA99841` · **Version:** `0.1.2` · **Developer:** YAHBALLON · **Language:** English.

## First-time setup

On first launch or after choosing **Settings → Run player setup again**:

1. Select the number of players (2–8) with Left/Right, then **X**.
2. Select each player, press **X** to edit their name with the on-screen DualSense keyboard (letters, numbers, space, backspace). **Options** or the keyboard **OK** key confirms. **Square** advances to the next setup page.
3. Choose **Standard**, **Wild**, **Couples**, **Quickfire**, or **Truth & Dare** mode. Mode choice affects eligible wheel segments and challenges.
4. Choose **Light**, **Normal**, or **All Challenges**. Each intensity has a real, filtered challenge pool.
5. Confirm the lobby summary with **X**. Setup and names persist in `/data/DrinkRoulette/settings.ini`.

## Gameplay and controller

- **Home:** D-Pad select Start Game, Players, Game Mode, Challenges, or Settings. **Triangle** shows Rules.
- **Players:** Up/Down select player, Square opens name keyboard, X toggles Ready, Left/Right changes player count.
- **Game:** X spins the bottle, rumble indicates result, X at result hands control to the next player, Circle returns to home.
- **Settings:** Change mode, challenge intensity, session length or replay player setup.
- **On-screen name entry:** D-Pad moves between keys, X types, Triangle deletes, Options saves, Circle cancels.
- Every drinking activity is optional, and non-alcoholic alternatives are encouraged.

### Challenge deck

433 unique offline prompts across Truth, Dare, Drink, Shot/Quickfire, Cheers, Group, and Pass. Recent prompts are tracked to reduce repetition. Gameplay modes filter the wheel and deck rather than simply altering labels. More than 400 prompts are available with Standard + All Challenges.

## Build / installation

GitHub Actions workflow: `.github/workflows/build-drink-roulette.yml`.

It clones the `Yahballon/PS5-DevBridge` native framework and compiles an installable ZIP after host regression tests. The OpenGL SDK v1.0.2 mirror is SHA-256 pinned because the original upstream download is unavailable. The output `PPSA99841.zip` is published at GitHub Releases and discovered by `tmp_lap/version.json` once catalog refresh runs. Unpack the title folder under `/data/homebrew/PPSA99841/` for a compatible homebrew loader or use the TESTBUILDS application.

This homebrew is for supported jailbroken PS5 consoles. Host and GitHub CI build success do not imply testing on physical PS5 hardware.

## Sources, assets and licensing

- `native/main.cpp`: PlayStation-style menus, controller handling and OpenGL wheel animation.
- `native/game.hpp`: standalone, testable game state and spin logic.
- `native/challenges.hpp`: 433 compiled-in challenge strings.
- `assets/menu_background.jpg`: updated user-approved neon lobby reference background.
- `assets/bottle.png`: original transparent glass bottle illustration for the spinning pointer.
- `assets/generate_bottle.py`: reproducible bottle-art generator.
- `tests/game_test.cpp`: onboarding, categories, spin and mode regression tests.

GPL-3.0-or-later (consistent with the native UI framework). No Sony or PlayStation marks are embedded.


## v0.1.3: Roulette display fix

The native renderer rotates texture UVs within the destination quad. The old non-square quad clipped the bottle as it spun. The bottle now uses a square transparent atlas with the center pivot aligned to the wheel. The supplied 1080p background contains no baked credits or version text; matching native overlay credits were also removed.
