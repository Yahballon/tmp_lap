// SPDX-License-Identifier: GPL-3.0-or-later
// Draws an ORIGINAL procedural 8-bit test pattern on the PS5 homebrew UI.
// Deliberately NOT a zplayer game frame. The future Allegro BITMAP adapter
// will feed real 8-bit frame+palette data to these same methods.
#include "engine/ps5_palette_texture.hpp"
#include "gfx/gl_batch.hpp"
namespace zcps5::engine {
class Ps5VideoProbe {
 public:
  bool init(hui::gfx::GlBatch& batch) {
    if (!frame_.configure(256,224)) return false;
    const Rgb6 pal[16] = {
      {0,0,0},{63,63,63},{50,40,6},{5,45,25},
      {7,26,63},{63,11,20},{43,8,55},{63,45,6},
      {32,32,32},{63,63,0},{0,63,63},{63,0,63},
      {63,32,0},{25,63,5},{32,8,63},{32,63,48}
    };
    for (int i=0;i<16;i++) frame_.set_palette(static_cast<std::uint8_t>(i),pal[i]);
    draw_pattern(0);
    return texture_.initialize(batch,frame_);
  }
  void draw_pattern(unsigned frame) {
    // Moving vertical line, quadrant/checker pattern and palette gradient.
    for(int y=0;y<224;y++) for(int x=0;x<256;x++) {
      int tile = ((x/16) ^ (y/16)) & 1;
      int color = ((x/32) + (y/28)*4 + tile*2) & 15;
      if (x == static_cast<int>(frame%256) ||
          x == static_cast<int>((frame+1)%256)) color=1;
      frame_.set_pixel(x,y,static_cast<std::uint8_t>(color));
    }
    texture_.refresh(frame_);
  }
  std::uint32_t texture() const { return texture_.texture(); }
  void release() { texture_.release(); }
 private:
  PalettedSurface frame_;
  Ps5PaletteTexture texture_;
};
} // namespace zcps5::engine
