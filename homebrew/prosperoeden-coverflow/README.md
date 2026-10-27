# ProsperoEden × PS5GBAX Coverflow

**Native PS5 homebrew UI port · by YAHBALLON · experimental v0.1.0**

This adapts the [PS5GBAX](https://github.com/Yahballon/PS5GBAX) Coverflow to the native [ProsperoEden](https://github.com/Swordpdf/ProsperoEden) launcher. It deliberately preserves the real Eden emulation engine and runtime, saves, controller service and title identity **PPSA99008**.

## What is implemented in source

- Animated large centered game case and receding, yawed side cases, dimming and floor reflections. The existing C++ OpenGL batch renderer adds a projected-image quad; it does not require a Vulkan renderer rewrite.
- Uses ProsperoEden's existing texture cache. Renders only the eleven nearest games. Empty libraries and missing art retain the existing fallback.
- Left/Right scroll one game, Up/Down jump five. Cross still launches/downloads, Triangle opens game settings, Square toggles mods/queues downloads, Circle goes back, and L1/R1 switches sections.
- The original top-navigation grid remains available. Existing details pane, remote progress, game saves and emulation functionality were not rewritten.

## Build and release workflow

[Native PS5 build workflow](../../.github/workflows/build-prosperoeden-coverflow.yml) checks out a pinned ProsperoEden revision, applies all five changed UI and GL files, runs geometry checks, installs ProsperoEden's actual Ubuntu CI SDK, compiles its original package, verifies eboot.bin, icon and metadata, and only if successful publishes an immutable GitHub Release with PPSA99008.zip. It then runs the repository's existing TESTBUILDS catalog generator; version.json is never hand-edited.

Original sources and revisions are recorded in [SOURCE.json](SOURCE.json).

**Until an actual GitHub Actions run succeeds, this is source work, not a downloadable PS5 installer.** Console rendering and game launch are not verified by CI. No console screenshot is available yet. Back up saves before installing a build sharing the upstream title ID.

## Run locally

Clone ProsperoEden and check out commit a26ba788222465ec7fb94dc813958fc749734e07. Then, from your tmp_lap checkout, run:

~~~sh
python3 homebrew/prosperoeden-coverflow/tools/apply_overlay.py --source /path/to/ProsperoEden
g++ -std=c++17 -O2 -Wall -Wextra -Werror homebrew/prosperoeden-coverflow/tests/native_coverflow_test.cpp -o /tmp/eden-coverflow-test
/tmp/eden-coverflow-test
~~~

On the supported Ubuntu native build host, from the patched ProsperoEden checkout run make. The overlay application refuses unexpected upstream revisions and dirty trees.

## Licensing, visuals, roadmap

GBAXY's Vulkan scene is not copied byte-for-byte: this is a 2.5D approximation in ProsperoEden's independent OpenGL UI. The exact native 3D case mesh and real PS5 screenshot are not yet available. Credits: ProsperoEden, its upstream dependencies and PS5GBAX contributors; GPL-3.0-or-later. Preserve upstream LICENSE and THIRD_PARTY_NOTICES.

[Milestones and next-agent handoff](MILESTONES.md).
