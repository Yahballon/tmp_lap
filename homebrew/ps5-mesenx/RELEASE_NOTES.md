# PS5MesenX v0.1.18 — HD pack memory safety and responsiveness

The normal NES renderer was working on PS5, while enabling large HD packs could
make the console unresponsive and trigger a page fault. This release changes
the HD path without replacing the working NES core.

## Changes

- Bounded PNG validation **before** allocating decoded image buffers (max
  4096×4096 and 64 MiB for a single decoded texture).
- Per-game decoded HD atlas budget of 256 MiB; invalid or over-budget atlases
  use NES fallback tiles rather than unsafe bitmap memory access.
- Validate HD tile dimensions, cropping coordinates, row strides and decoded
  image bounds before touching pixels; an empty tile never dereferences index 0.
- Do not decode all atlases in parallel at load. Sequentially prewarm up to
  64 MiB, defer remaining atlases until the game uses them.
- Avoid copying the entire compressed PNG for every decode.
- Keep NES emulation/audio running at its native rate while throttling HD
  presentation to at most 30fps. Large HD frames are downsampled to no more
  than 1280×1200 before creating Vulkan textures. Normal NES video is unchanged.
- Log HD atlas allocation denial, missing textures and HD-frame reductions for
  diagnosing compatibility with individual packs.

## Compatibility and limitations

An HD pack is **not guaranteed** to work just because it is under 100 MB on
disk; decompressed RGBA textures and tile copies can occupy much more RAM.
Very large packs may show the original NES graphics for some tiles when
memory budget limits are reached. This is preferable to a page fault. A
future native renderer can add streaming and a proper texture-cache eviction
policy. Performance and PS5 OS overlay behavior need on-console verification.

Install: `/data/homebrew/PPSA99896/`.
NES ROMs: `/data/PS5NES/games/`.
HD packs: `/data/PS5MesenX/HdPacks/<ROM stem>/hires.txt`.
Log: `/data/PS5MesenX/logs/boot.log`.
