# ProsperoEden × GBAXY Coverflow

**Native PS5 homebrew UI integration — by YAHBALLON**

This is a source-stage port of the [PS5GBAX](https://github.com/Yahballon/PS5GBAX) coverflow layout concept into [Swordpdf/ProsperoEden](https://github.com/Swordpdf/ProsperoEden). **Not yet a compiled app**, and not yet published to TESTBUILDS. The upstream ProsperoEden repository grants this GitHub connection read-only access.

## Technical baseline

- ProsperoEden main tree examined at `a26ba788222465ec7fb94dc813958fc749734e07`.
- PS5GBAX source examined at `197c62e08cd709299785fa07c8c0addbb844c51d`.
- ProsperoEden uses C++ OpenGL 2D DrawList and GlBatch for its launcher; PS5GBAX uses Vulkan/3D cover case transforms. The correct approach is native geometry adaptation, **not** wholesale copying of Vulkan renderer.
- The portable GBAXY-inspired pose model is in [coverflow_layout.hpp](coverflow_layout.hpp). Cases get fractional scrolling, larger center, scaled/angled sides, dimming and hover. No copyrighted game covers are included.
- Original ProsperoEden's Title ID `PPSA99008` and existing save data paths must be preserved.

## Files to integrate in the pinned ProsperoEden clone

- `headless/prosperoeden/pe/ui/library.cpp`: replace non-top-navigation four-column library with coverflow; use `pose(index, grid_x_.value / kCellPitchX, time_)` to animate cases. Keep the detail pane, cover cache, remote download status and all original game actions.
- `headless/prosperoeden/pe/ui/launcher.hpp`: optionally use existing `grid_x_` spring; the selected item index should be the target for fractional animation, not selected column.
- `headless/prosperoeden/pe/gfx/draw_list.{hpp,cpp}` and `pe/gfx/gl_batch.cpp`: add a perspective-cover draw shape to the existing GL batch; preserve shaders used by other views. The layout model provides signed yaw from -0.82 to +0.82.
- Keep the existing top-navigation theme layout unchanged.
- Reuse `Textures::cover()` and `pump()`, which already provide async artwork and GPU cache eviction.

Recommended controller mapping: Left/Right one case, Up/Down jump five, Cross play, Triangle settings, Square existing mods/download action, Circle back.

## Test the geometry

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Werror homebrew/prosperoeden-coverflow/tests/coverflow_geometry_test.cpp -o /tmp/coverflow-test
/tmp/coverflow-test
```

Native PS5 compilation still requires ProsperoEden's pinned SDK and dependencies under Ubuntu 26.04/WSL (`make` from the upstream checkout). The current integration needs source-file implementation beyond the portable geometry header before building an installable ZIP. **No screenshot, successful PS5 build, or release has been verified.**

Credits: ProsperoEden contributors and PS5GBAX contributors. GPL-3.0-or-later. See [MILESTONES.md](MILESTONES.md) for handoff and next engineering action.
