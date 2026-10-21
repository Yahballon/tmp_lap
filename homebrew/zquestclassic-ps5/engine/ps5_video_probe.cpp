// SPDX-License-Identifier: GPL-3.0-or-later
// Deliberately original procedural test pattern, NOT a zplayer game frame.
#include "engine/ps5_video_probe.hpp"
#include "gfx/gl_batch.hpp"
namespace zcps5::engine {
bool Ps5VideoProbe::init(hui::gfx::GlBatch& batch) {
  if (!frame_.configure(256,224)) return false;
  const Rgb6 pal[16] = {
    {0,0,0},{63,63,63},{50,40,6},{5,45,25},
    {7,26,63},{63,11,20},{43,8,55},{63,45,6},
    {32,32,32},{63,63,0},{0,63,63},{63,0,63},
    {63,32,0},{25,63,5},{32,8,63},{32,63,48}
  };
  for (int i=0;i<16;i++) frame_.set_palette(static_cast<std::uint8_t>(i),pal[i]);
  // Populate before allocating GL storage.
  draw_pattern(0);
  return texture_.initialize(batch,frame_);
}
void Ps5VideoProbe::draw_pattern(unsigned frame) {
  // 8-bit striped/checker palette test and animated white vertical scanline.
  for (int y=0;y<224;y++) for (int x=0;x<256;x++) {
    const int checker = ((x/16) ^ (y/16)) & 1;
    int idx = ((x/32) + (y/28)*4 + checker*2) & 15;
    if (x == static_cast<int>(frame%256) ||
        x == static_cast<int>((frame+1)%256)) idx = 1;
    frame_.set_pixel(x,y,static_cast<std::uint8_t>(idx));
  }
  texture_.refresh(frame_);
}
} // namespace zcps5::engine
