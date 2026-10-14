# Starbound Rebased v0.1.24 — DualSense Settings TESTBUILD

- Replaced the blocking keyboard-only settings capture with per-frame controller/keyboard capture. Circle cancels capture immediately and returns from settings; Options pauses gameplay and can also cancel a pending capture.
- Cross confirms; D-pad and left stick navigate. Gameplay defaults: Cross jumps, Square fires and sprints, R1 also sprints, D-pad/left stick moves and crouches.
- Settings show real PS5 control names. All six gameplay actions can be rebound to controller buttons or D-pad/stick directions. Release the opening Cross first, then press the new control. Circle and Options stay reserved. Shared bindings are allowed for run/fire.
- Triangle restores controller defaults while browsing settings. Rebindings last for the current game session; reopening settings retains them, restarting the application restores defaults.
- Disconnected controllers release held actions; reconnection scans all available controllers. Keyboard Escape cancellation and keyboard rebinding remain functional on desktop.
- Compiled regression tests exercise the actual Input implementation and settings handlers with a virtual SDL controller, including cancel, opening-button release, remapping, reset, menu/action separation, disconnect and keyboard fallback.

The 32-stage campaign, dense encounters, moving platforms, underwater stages, bosses, HD assets and repaired player sprites are preserved. Install PPSA99642.zip through ShadowMount. Keep v0.1.23 for rollback. PS5 hardware validation remains necessary: test Circle cancel, remap Jump, reset with Triangle, then pause with Options.

# Starbound Rebased v0.1.23 — Moving Frontiers TESTBUILD

- 10–19 enemies in every stage: 421 authored spawns across 32 levels, with approach
  patrols, exit patrols, elevated guards and denser submerged encounters. Entry,
  checkpoint, boarding and landing buffers remain clear.
- 54 real moving platforms. All 28 dry stages require boarding a lift, riding
  upward and jumping onto a high ledge.12 cargo/storm/core stages additionally
  require a horizontal ferry across a 28-tile fissure.
- Replaced static 609 campaign tiles with genuine three-tile 809 platform entities.
  Also repaired the engine's one-tile609 factory for future compatible maps.
- Platforms clamp at their endpoints and keep cycling off-camera so a missed
  ferry returns. Horizontal shuttles move faster; vertical lifts remain gentle.
- Player transport uses actual displacement and top contact. Side contact no
  longer drags the player, and descending lifts retain grounded jump input.
- Continuous native48px deck artwork makes the entire96px landing area visible.
- Route checks require dynamic boarding/ride/exit paths and reject dry-level
  completion with movement disabled. Compiled engine tests cover both axes,
  both player sizes, entity ordering, reversals, jump-off and off-camera cycling.

Controls, repaired walking/growth sprites, biome backdrops, underwater mode,
checkpoints and sentinel shutdowns are retained. Install PPSA99642.zip through
ShadowMount; keep v0.1.22 for rollback. Hardware-test boarding, waiting for the
ferry to return, large-player headroom, descending-lift jumps and enemy pacing.

# v0.1.22 — Deep Signal placed-sentry correction

Retains the complete Deep Signal campaign and artwork. Placed air sentries now
survive offscreen until their first activation, then expire when leaving view.
Added a compiled regression using the real spawn block, PhysicsSystem and
CallbackSystem. Supersedes v0.1.21 for testing the new air enemies.

# Starbound Rebased v0.1.21 — Deep Signal TESTBUILD

Relayfall now has 32 larger, deliberately sequenced stages, with unique six-bay
encounter plans instead of repeated half-level templates.

- Four underwater observatory stages with actual repeated-jump swimming, coral
  channels, submerged caches and surface airlocks. Swimming now survives damage,
  checkpoint death and transitions between dry and submerged sectors.
- Two reactor-sentinel arenas (6-4 and 8-4). Reach the marked cut-off terminal
  via the upper maintenance route to collapse the bridge and shut down the boss.
- Newly introduced glow rays, jelly drones, animated air sentries and a connected
  32x32 reactor sentinel, with original generated/normalized sprite artwork.
- Three original biome backdrops and distinct quarry/coral/reactor terrain.
- New tunnel, island-valley, sky-bridge and drone encounters, safe checkpoint
  buffers and optional reward routes.
- Existing player walking/growth fixes, native atlas geometry and controls retained.
- Automated map, swim-corridor, boss-assembly, actual parser and packaged-art gates.

Install PPSA99642.zip with the existing ShadowMount workflow. Hardware testing is
required for underwater damage/retries, swim-to-dry transitions, enemy animation,
cut-off terminal timing and the final confirmation back to the menu. Keep v0.1.20
available for rollback. See CAMPAIGN.md, DESIGN_RESEARCH.md and ASSET_NOTES.md.

# v0.1.20 — Relayfall campaign TESTBUILD

- Replaced all 32 level layouts with an original ordered sci-fi campaign.
- Restored the full 8x4 level menu and sequential sector transitions.
- Added session checkpoints with safe large-player spawn/camera restoration.
- Added optional lift/cache routes, meaningful item placement, real enemy IDs,
  conservative difficulty progression and a final signal-restored prompt.
- Corrected CAMERA_MAX authoring to use the absolute map boundary.
- Added map semantics, static small/large route checks, negative regression tests
  and packaged-map parity gates before the prerelease TESTBUILD publication.
- Preserved player animation/growth fixes, HD media, controls and physics.
- PS5 hardware playtesting remains required; see CAMPAIGN.md.


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

## v0.1.17 — AI characters in original source rectangles

Adds the actual developer-supplied pink astronaut and enemy art to original Gold872 small 16x16, large 16x32 and enemy source IDs; draws cyan projectiles at fireball IDs 246/247. Uses the gold reference 1x sprite canvas dimensions and unchanged world collision geometry. Block background transparency/colour key continue to be verified against Gold872. Experimental hardware build; PS5 screenshots needed.

## v0.1.18 — Distinct Starbound visual identity

- Completely replace the prior palette-shifted Mario-compatible block drawings with original sci-fi energy crates, crystal terrain, neon panel technology and conduits while retaining the same gameplay tile IDs and original canvas.
- Suppress old cloud/hill background sprite IDs where not used by foreground and render a bespoke deep-space night skyline (alien settlement, stars, ringed planet).
- Replace remaining Mario-character pixels by populating all player frame families with the user-supplied pink astronauts (plus cohesive 16x32 forms and cyan fireball IDs 246/247). Fill enemy atlas with user-provided slime, armored, drone and plant variants.
- Replace the NES-style HUD font with Liberation Sans Bold and WORLD label with SECTOR.
- Keep PS5 input, collision, game physics, World1-1 original level property/CSV and all rendering addresses untouched. Add CI packaging checks for all art sheets, semantic tiles and new sky.
