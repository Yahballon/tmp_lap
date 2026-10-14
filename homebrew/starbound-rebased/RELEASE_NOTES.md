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
