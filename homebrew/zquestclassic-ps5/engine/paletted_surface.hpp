// SPDX-License-Identifier: GPL-3.0-or-later
// Indexed 8-bit bitmap and RGB 6-bit palette bridge for a future Allegro
// legacy BITMAP -> native PS5 OpenGL compositor. No zplayer frame hookup yet.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace zcps5::engine {
struct Rgb6 {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
};

class PalettedSurface {
 public:
  static constexpr int kMaxDimension = 2048;
  bool configure(int width, int height) {
    if (width < 1 || height < 1 || width > kMaxDimension ||
        height > kMaxDimension) return false;
    const std::size_t pixels =
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (pixels > std::numeric_limits<std::size_t>::max() / 4) return false;
    w_ = width;
    h_ = height;
    indices_.assign(pixels, 0);
    rgba_.assign(pixels * 4, 0);
    dirty_ = true;
    return true;
  }
  int width() const { return w_; }
  int height() const { return h_; }
  static std::uint8_t from_allegro6(std::uint8_t c) {
    if (c > 63) c = 63;
    return static_cast<std::uint8_t>((static_cast<unsigned>(c) * 255u + 31u) / 63u);
  }
  void set_palette(std::uint8_t index, Rgb6 rgb) {
    const auto to8 = [](std::uint8_t v) { return from_allegro6(v); };
    const std::array<std::uint8_t, 4> rgba = {
      to8(rgb.r), to8(rgb.g), to8(rgb.b), 255
    };
    if (palette_[index] != rgba) {
      palette_[index] = rgba;
      dirty_ = true;
    }
  }
  void clear(std::uint8_t index) {
    for (auto& v : indices_) v = index;
    dirty_ = true;
  }
  bool set_pixel(int x, int y, std::uint8_t index) {
    if (x < 0 || y < 0 || x >= w_ || y >= h_) return false;
    indices_[static_cast<std::size_t>(y) * static_cast<std::size_t>(w_) + x] = index;
    dirty_ = true;
    return true;
  }
  // Accept an Allegro-style row pitch that may exceed bitmap width.
  // Length is the actual readable source allocation, not assumed.
  bool upload_indices(const std::uint8_t* source, std::size_t source_size,
                      std::size_t pitch) {
    if (w_ < 1 || h_ < 1 || !source || pitch < static_cast<std::size_t>(w_))
      return false;
    const std::size_t row = static_cast<std::size_t>(h_ - 1);
    if (row && pitch > (std::numeric_limits<std::size_t>::max() -
                        static_cast<std::size_t>(w_)) / row)
      return false;
    if (row * pitch + static_cast<std::size_t>(w_) > source_size) return false;
    for (int y = 0; y < h_; ++y)
      for (int x = 0; x < w_; ++x)
        indices_[static_cast<std::size_t>(y) * w_ + x] =
          source[static_cast<std::size_t>(y) * pitch + x];
    dirty_ = true;
    return true;
  }
  // Converts source indexes and current palette to a stable RGBA8 buffer.
  const std::vector<std::uint8_t>& rgba() {
    if (dirty_) {
      for (std::size_t i = 0; i < indices_.size(); ++i) {
        auto& entry = palette_[indices_[i]];
        rgba_[i * 4] = entry[0];
        rgba_[i * 4 + 1] = entry[1];
        rgba_[i * 4 + 2] = entry[2];
        rgba_[i * 4 + 3] = entry[3];
      }
      dirty_ = false;
    }
    return rgba_;
  }
 private:
  int w_ = 0, h_ = 0;
  std::vector<std::uint8_t> indices_, rgba_;
  std::array<std::array<std::uint8_t, 4>, 256> palette_{};
  bool dirty_ = true;
};
} // namespace zcps5::engine
