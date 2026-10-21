// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Source photo's actual hole centres, independently detected from 42 rings.
// Virtual canvas: 1920 x 1080. Row index 0 is the game engine's bottom row.
#include <array>
namespace wins4 {
constexpr std::array<float,7> kHoleCentersX{{648.0f,756.0f,864.0f,973.0f,1081.0f,1189.0f,1297.0f}};
constexpr std::array<float,6> kHoleCentersTopY{{288.0f,380.0f,472.0f,565.0f,659.0f,759.0f}};
constexpr float kDiscSize=77.0f; // fits the original hole's inner diameter
constexpr float hole_x(int col) {return kHoleCentersX[col];}
constexpr float hole_y(int bottom_row) {return kHoleCentersTopY[5-bottom_row];}
}
