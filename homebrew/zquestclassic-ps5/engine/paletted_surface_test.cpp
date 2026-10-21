// SPDX-License-Identifier: GPL-3.0-or-later
#include "paletted_surface.hpp"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <limits>
int main() {
  using namespace zcps5::engine;
  PalettedSurface fb;
  assert(!fb.configure(0, 240));
  assert(!fb.configure(2049, 240));
  assert(fb.configure(2, 2));
  assert(PalettedSurface::from_allegro6(0) == 0);
  assert(PalettedSurface::from_allegro6(63) == 255);
  assert(PalettedSurface::from_allegro6(31) == 125);
  assert(PalettedSurface::from_allegro6(90) == 255);
  fb.set_palette(0, {0,0,0});
  fb.set_palette(1, {63,0,31});
  fb.set_palette(2, {0,63,0});
  // Two rows of two indexed pixels; source pitch contains padding bytes.
  const std::array<std::uint8_t,8> indices = {0,1,99,99,2,1,99,99};
  assert(!fb.upload_indices(indices.data(), indices.size(), 1));
  assert(!fb.upload_indices(indices.data(), 5, 4));
  assert(!fb.upload_indices(indices.data(), indices.size(),
                            std::numeric_limits<std::size_t>::max()));
  assert(fb.upload_indices(indices.data(), indices.size(), 4));
  const auto& out = fb.rgba();
  assert(out.size() == 16);
  assert((out[0] == 0 && out[3] == 255));
  assert((out[4] == 255 && out[5] == 0 && out[6] == 125 && out[7] == 255));
  assert((out[8] == 0 && out[9] == 255 && out[10] == 0));
  // Changing the palette must update existing pixels without reupload.
  fb.set_palette(1, {0,0,63});
  const auto& changed = fb.rgba();
  assert(changed[4] == 0 && changed[6] == 255);
  assert(!fb.set_pixel(-1,0,1));
  assert(!fb.set_pixel(2,0,1));
  assert(fb.set_pixel(0,0,2));
  assert(fb.rgba()[1] == 255);
  fb.clear(1);
  assert(fb.rgba()[2] == 255);
  puts("PASS: 6-bit Allegro palette, pitched 8-bit BITMAP, RGBA8 and bounds");
}
