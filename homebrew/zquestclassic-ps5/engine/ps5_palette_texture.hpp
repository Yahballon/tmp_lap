// SPDX-License-Identifier: GPL-3.0-or-later
// GPU upload adapter for the existing PS5-DevBridge EGL/OpenGL renderer.
#pragma once
#include "engine/paletted_surface.hpp"
#include <cstdint>
namespace hui::gfx { class GlBatch; }
namespace zcps5::engine {
class Ps5PaletteTexture {
 public:
  bool initialize(hui::gfx::GlBatch& batch, PalettedSurface& frame);
  bool refresh(PalettedSurface& frame);
  void release();
  std::uint32_t texture() const { return texture_; }
 private:
  std::uint32_t texture_ = 0;
  int width_ = 0, height_ = 0;
};
} // namespace zcps5::engine
