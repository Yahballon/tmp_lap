# Starbound Sprint

Native 2D side-scrolling PS5 homebrew platform game using original procedural geometry. No Mario characters, maps, art or sound are included.

**v0.2.0 · PPSA99641 · Content version 01.000.002**

Eight stages, two jumps per landing (double jump), HUD air-jump counter, platform physics, camera scrolling, collectible crystals, roaming drones, springs, checkpoints, score, lives, progressive unlocks, and saved score.

## Controller

| Button | Use |
| --- | --- |
| D-pad / Left Stick | Move |
| Cross | Jump; release and press again in midair for a second jump; confirm |
| Square | Sprint |
| Options | Pause / resume |
| Circle | Back; exit from title |
| Triangle | Continue from most recently unlocked world |

## Installation

Download PPSA99641.zip from the versioned GitHub release, extract PPSA99641 into /data/homebrew/, and register with ShadowMountPlus or a compatible PS5 homebrew loader.

## Build

See [dedicated GitHub Actions workflow](../../.github/workflows/build-starbound-sprint.yml). Uses GPL-3.0-or-later ps5-homebrew-ui via the PS5-DevBridge simple-calc PS5 toolchain.

## Double jump

Press Cross once to jump, release it and press again before landing for one extra midair jump. A third press while still airborne does nothing. Both jumps recharge on landing. Launch pads and enemy stomps allow one midair jump afterward. The HUD displays the remaining jump count.

## Persistence

Progress is stored in /data/homebrew/PPSA99641/starbound-progress.cfg. Preserve that file when replacing the application directory during upgrades.

## Limitations

No audio, animated sprite sheet or real-PS5 hardware testing yet. The first release uses native procedurally drawn shapes. License: GPL-3.0-or-later.
