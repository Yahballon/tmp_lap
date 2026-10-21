// SPDX-License-Identifier: GPL-3.0-or-later
// Staging-only PS5 graphics proof: 256x224 indexed -> RGBA8 GL texture.
#pragma once
#include "engine/paletted_surface.hpp"
#include "engine/ps5_palette_texture.hpp"
#include <cstdint>
namespace hui::gfx { class GlBatch; }
namespace zcps5::engine {
class Ps5VideoProbe {
 public:
  bool init(hui::gfx::GlBatch& batch);
  void draw_pattern(unsigned frame);
  std::uint32_t texture() const { return texture_.texture(); }
  void release() { texture_.release(); }
 private:
  PalettedSurface frame_;
  Ps5PaletteTexture texture_;
};
} // namespace zcps5::engine
