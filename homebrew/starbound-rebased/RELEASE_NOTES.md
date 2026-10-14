# Starbound Rebased v0.1.8 — PS5 absolute resource path resolution

Real PS5 hardware shows the original Gold872 game loop and SDL_RenderPresent work, but startup logs report all four resource probes missing: title logo, fonts, menu map and music. The ZIP content is correct, but the existing `chdir("/app0/assets")`-based loader does not provide working relative paths in the user's environment.

This release replaces every upstream C++ `"res/..."` resource filename with a resolver which probes **real, readable files** under `/app0/assets/`, `/data/homebrew/PPSA99642/assets/`, common USB/SSD locations, and both working-directory variants. Only a successful `fopen` of the packaged menu logo selects a root. The same absolute root is then used for SDL textures, fonts, C++ level CSV files, images, music and audio. At PS5 boot the existing log includes `ASSET_ROOT_FOUND <path>` or `ASSET_ROOT_MISS <path>` for every candidate checked.

No `chdir()` or `getcwd()` dependencies remain in game startup. CI ensures the original engine's relative file paths are all rewritten and that the correct files are in the installable ZIP. This is a **candidate hardware fix**; only actual PS5 testing confirms whether menu art appears.
