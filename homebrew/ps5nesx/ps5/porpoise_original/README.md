# Original Project Porpoise renderer port for PS5NESX

**Experimental feature branch, NOT yet a verified PS5 release.**

Goal: Render the NES game library using the **original** Project Porpoise
Gfx Vulkan implementation (including its original SPIR-V shader bytecode),
rather than imitating it with the legacy black plastic NES cover mesh.

Original sources from
[elripalda/Porpoise-Dolphin-Emulator-for-PS5](https://github.com/elripalda/Porpoise-Dolphin-Emulator-for-PS5),
commit `2ed1ab5a06995aa198f7bccd4da0f90c3fc29d39`.
Source files are imported **byte-for-byte**, retaining the original
copyright headers and GPL-3.0-or-later licence. Their Git blob hashes
are verified during CI. Author: Ruben (Project Porpoise).

The NES bridge `../frontend/fe_exact_porpoise.{h,cpp}` is a new adapter;
unlike the upstream UI source, it must translate the NES game library to
the upstream renderer. It uses Porpoise's exact 1920x1080 coverflow
coordinates, front-cover glass block, reflection and cover artwork
geometry, including its source values and animations. The remaining NES
game details and Sony controller-specific settings are intentionally
PS5NESX's, not Dolphin emulator functions.

The build downloads the original Porpoise Nunito fonts and icons at a
**pinned revision**, verifies original Git blob hashes and bundles the UI
assets so the renderer can read them at `/app0/porpoise-ui/`.
The native Gfx renderer uses the precompiled original shader in
`src/ui_frag.inc`; it does not use our old NES black-plastic shader.
Included fonts and icons retain the original licences.

Fallback: If the original renderer fails to initialize, PS5NESX uses
the existing v1.0.15 Vulkan menu. The emulator's NES gameplay renderer
remains unchanged even when the original UI initializes.

Must test on an actual PS5 before release: renderer init, Vulkan driver
compatibility, frame latency, font and icon loading, original cover
perspective, reflection, input sounds, launching and exiting the game.
This is *not yet* a guarantee of pixel-perfect parity.
