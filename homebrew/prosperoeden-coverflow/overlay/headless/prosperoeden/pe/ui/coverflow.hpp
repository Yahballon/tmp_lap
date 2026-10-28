// GBAXY-inspired coverflow layout, tuned from PS5 console screenshot.
// Preserve square cover-art aspect; do not force portrait art into Switch covers.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>
namespace pe::ui::coverflow {
struct Pose { float x, y, w, h, yaw, brightness, opacity; };
inline Pose pose(float index, float scroll, float time,
                 float center_x=785.0f, float floor_y=692.0f) {
    const float delta = index - scroll;
    const float distance = std::fabs(delta);
    const float sign = delta < 0.0f ? -1.0f : 1.0f;
    const float n = std::clamp(distance, 0.0f, 1.0f);
    const float side = n * n * (3.0f - 2.0f * n);
    // Compress the rear cases into the visible Library area. Previously
    // each further case moved 138px outwards and was clipped by the detail pane.
    const float tail = std::min(3.0f, std::max(0.0f, distance - 1.0f));
    const float outer = std::max(0.54f, 0.84f - 0.10f * tail);
    const float scale = 1.28f + (outer - 1.28f) * side;
    const float extent = 300.0f * scale;
    const float travel = side * (305.0f + 50.0f * tail);
    const float hover = (1.0f - side) * (5.0f + 2.5f * std::sin(time * 1.1f));
    return {center_x + sign * travel - extent * 0.5f,
            floor_y - extent - hover,
            extent, extent, -sign * 0.62f * side,
            std::max(0.60f, 1.0f - 0.12f * side - 0.055f * tail),
            std::max(0.16f, 1.0f - 0.11f * tail)};
}
}
