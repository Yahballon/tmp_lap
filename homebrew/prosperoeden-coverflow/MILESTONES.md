# ProsperoEden GBAXY Coverflow — Milestones & Handoff

Updated: 2026-10-27 UTC
Branch: `Yahballon/tmp_lap:feature/prosperoeden-gbaxy-coverflow`
Upstream source: `Swordpdf/ProsperoEden@a26ba788222465ec7fb94dc813958fc749734e07`
Design donor: `Yahballon/PS5GBAX@197c62e08cd709299785fa07c8c0addbb844c51d`
Title ID: `PPSA99008` (existing, retain)
Release: NONE; hardware-tested: NO; screenshot: NOT CAPTURED

## Status

| Milestone | Evidence | Status |
| --- | --- | --- |
| Architecture review | Inspected both repos' current source, OpenGL vs Vulkan rendering, library scanning and controls | VERIFIED |
| Geometry model | `coverflow_layout.hpp` added with deterministic position/scale/yaw | IMPLEMENTED / UNVERIFIED |
| Geometry regression | `tests/coverflow_geometry_test.cpp` | PREPARED / NOT RUN |
| Native Eden library integration | `library.cpp` source still needs applying | NOT STARTED |
| New GL shape / shader case | `gl_batch.cpp` source still needs applying | NOT STARTED |
| PS5 build + packaging | No full emulator SDK / workflow result | NOT STARTED |
| Real console test + TESTBUILDS | No verified build artifact | NOT STARTED |

## Observations and boundaries

The upstream native launcher lives under `headless/prosperoeden/pe/ui/`; `library.cpp` currently draws the default four-column library plus an independent `top_nav` grid, and uses an existing animated spring, `grid_x_`. It already supports asynchronous game covers via `Textures`, remote downloads, game details, saves, and settings. PS5GBAX's donor coverflow pose is in `ps5/frontend/fe_coverflow_layout.h`, a Vulkan 3D scene that cannot be directly copied into ProsperoEden's instanced OpenGL 2D UI.

GitHub connection cannot push to Swordpdf/ProsperoEden. This work belongs in the user's writable experimental branch, **not in the upstream main branch**. No hand-edited TESTBUILDS `version.json`; its GitHub Action is the source of truth.

## NEXT ACTION FOR THE NEXT AGENT

**First:** take this geometry model and implement an OpenGL projected-image draw mode in ProsperoEden's `DrawList` / `GlBatch`, replacing the non-`top_nav` part of `Launcher::draw_library` while leaving top-nav and emulator behavior untouched. Verify the C++ host geometry test, then build the patched upstream with `make` on Ubuntu 26.04. After native ZIP validation, use a real PS5 to test movement, selection, downloads and game launch. No release claim before this succeeds.

Regression focus: Title ID `PPSA99008`, saves, remote downloads, quick selection changes, cover cache, missing artwork, settings, and launch transitions.
