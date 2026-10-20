# SNAKE by YAHBALLON — v0.1.0

An offline, controller-first native PS5 Snake game featuring a 1920 × 1080 neon jungle aesthetic inspired by the provided screenshot. Not a browser game or static mockup.

## Included

- Live 24 × 24 snake grid with pixel-art snake, glowing frame, food sprite, current score and locally saved high score.
- Three game modes: **Classic** (collision with walls and body), **Zen** (pass-through / no collisions), and **Sprint** (higher movement speed).
- Eight selectable speed levels, walls ON/OFF and three food styles (Apple, Cherry, Golden), including differing score rewards.
- Main menu, game mode selection, settings, high scores, pause, game over and exit confirmation.
- Analog / D-pad movement, sensible debouncing and options menu.
- Procedurally generated original tropical jungle background, neon SNAKE logo, pixel cobra silhouette and homebrew tile.
- Atomic saved preferences and high score in `/data/SnakeGame/save.ini`; debug log in `/data/SnakeGame/game.log`.

## DualSense controls

| Screen | Button | Action |
| --- | --- | --- |
| Main menu | D-pad / stick up/down | Select Play Game, Game Mode, Settings, High Scores, Exit |
| Main menu | Cross | Enter selected section |
| Main menu | Right | Focus quick settings; Up/Down select and Cross/Right change |
| Main menu | Square | Immediately start game |
| Game | D-pad / left stick | Turn the snake (180° reversal blocked) |
| Game | Options / Cross / Circle | Pause |
| Pause | Cross / Options | Resume |
| Pause | Triangle | New game |
| Pause | Circle | Menu |
| Game over | Cross / Circle | Retry / menu |
| Settings | Up / Down | Highlight setting |
| Settings | Left / Right | Change setting |
| Elsewhere | Circle | Back |

One controller is sufficient.

## Build

GitHub Actions: `.github/workflows/build-snake.yml`. It checks out the maintained `Yahballon/PS5-DevBridge` native `templates/simple-calc` framework, stages `native/main.cpp` and `native/game.hpp`, generates graphics with Pillow, prepares the native OpenGL SDK, compiles PS5 `eboot.bin` and packages `PPSA99846.zip`.

Host tests:

```sh
g++ -std=c++20 -O2 -Wall -Wextra -Werror homebrew/snake/tests/game_test.cpp -o /tmp/snake-tests
/tmp/snake-tests
```

The install archive contains `PPSA99846/eboot.bin`, `sce_sys/param.json`, `sce_sys/icon0.png`, `sce_module/libc.prx`, the UI font and `assets/jungle.jpg`. Place the **PPSA99846** folder in `/data/homebrew/` using an existing compatible homebrew loader. No payload, network access or account needed.

## Licensing & status

Original game code and generated art: GPL-3.0-or-later. Third-party PS5-DevBridge and ps5-homebrew-ui sources retain their own license notices. Source is not an installable PS5 binary: use the **verified CI ZIP** once its build is successful. PS5 console hardware test requires an actual PS5 and is not implied by CI.
