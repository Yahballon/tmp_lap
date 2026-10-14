# Starbound Rebased v0.1.12 — World 1-1 compatibility test

This is a **temporary technical reference build** for diagnosing the broken v0.1.11 environment art and misaligned asset IDs. 

- The entire World 1-1 level data (six CSV layers and exact levelproperties) is fetched from the pinned Gold872 engine repository revision at CI build time and compared byte-for-byte with the packaged data.
- Fetch the matching original character, enemy and block sprite sheets from the same revision, apply deterministic recoloring while retaining every sprite-grid cell, alpha mask, chroma key and original frame mapping, then upscale spritesheets 2x with nearest-neighbor (the engine's source rectangles are already scaled accordingly).
- Copy IrregularReferences.blockmap, needed for tile interaction and reference translation.
- Fix the **real C++ menu/logo bug**: unframed images use SDL_QueryTexture to read the full source dimensions, instead of cropping to the on-screen destination dimensions.
- Use the original plain background during the 1-1 test to remove semi-transparent dark-oval sky artifacts.
- Limit menu level selection to World 1-1; other independently authored test levels remain packaged for crash-safe transitions after completion.
- Preserve the working native PS5 800x480 offscreen renderer from v0.1.10 and the YAHBALLON HUD.
- New art in this test is a procedural recolor, **not AI-generated**. The public upstream repository's source-code license does not automatically grant rights to redistribute the Mario-themed art or level layout; this build needs an artwork/content rights review before public/general distribution.

**Validation:** CI compile/package and byte-match verification, plus PS5 runtime test for interactions, warp, score/HUD, options and menus. Do not claim runtime verified until tested on the console.


## v0.1.13 — optional HD art loader

The native PS5 resource resolver now accepts a complete **separate original-art HD sprite pack** at `/mnt/usb0/homebrew/starbound-rebased-hd/res/sprites/` or `/data/starbound-rebased/hd/res/sprites/` without changing the Gold872 level data, game physics, tile IDs or tested fallback resources. The new HD background is drawn only when that complete pack is detected. Existing installs without an HD pack retain the v0.1.12 appearance. The image assets themselves are distributed separately and are **not in the GitHub build**.

## v0.1.14 — embedded developer HD sprites (PS5 TESTBUILDS)

This release **actually contains** the six new original PNG graphics as part of the installable `PPSA99642.zip`. Previous v0.1.13 had only an optional loader and silently fell back to the old art.
The installed player, enemy, tiles, pink/cyan logo and app icon are from the provided artwork. The sky renders by default when the image loads. The build copies sprites after any legacy atlas generator and checks all six PNGs byte-for-byte inside the ZIP. Preserves fixed sprite atlas dimensions, World1-1 level CSV, controller and PS5 diagnostics. Experimental; verify menu/world/characters against screenshots on console.

## v0.1.15 — PS5 Screenshot Alignment Fix

- Corrected the 16px engine frame Y offset for HD player animation atlas: source (1,9) now maps to (2,18) as required by Gold872 (previous v0.1.14 was offset by 16 HD pixels per row).
- Aligned all enemy frames to the source atlas origin; composed tall armored enemies across contiguous 2x height, restored slime animations for enemies 70/71 in World1-1.
- Reduced landscape clutter and cloud/HUD overlap; dark blue HUD backdrop and scenery behind the game.
- Ground ID 384 now renders as solid pink crystalline rock; no grass texture in every stacked layer. Stone obstacle ID 289 draws a matching pink tile.
- Bundled resources now take priority over outdated external USB override packs.
- CI requires correct sprite source-frame coverage and embedded image byte-for-byte consistency before the automated GitHub release.

## v0.1.16 — restore Gold872 native atlas geometry

The earlier AI atlases used wrong source cell ranges: 2x grid with graphics in incompatible slots. This release rebuilds 1x sprite atlases guided by the pinned original PNG transparency (including original 17-pixel block gutters, 9-pixel player source offset and multi-row 16x32 sprites), recolors each occupied pixel into a pink/cyan theme, and restores 1x C++ source rectangles. No double-sized player sprites, humanoid projectiles or opaque replacement tiles from wrong atlases. The original provided logo, app badge and shaded background stay in place. CI now asserts actual PNG dimensions and byte-exact alpha masks against the source reference for all cells.
