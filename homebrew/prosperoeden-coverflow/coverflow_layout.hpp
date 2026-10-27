// GBAXY style coverflow projection parameters for ProsperoEden OpenGL launcher
// Copyright (C) 2026 YAHBALLON
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>
namespace pe::ui::gbaxy {
struct Pose {
    float x, y, width, height, yaw, brightness, opacity;
};
inline Pose pose(float game_index, float scrolling_index, float seconds,
                 float center_x = 780.f, float floor_y = 692.f) {
    const float offset = game_index - scrolling_index;
    const float distance = std::fabs(offset);
    const float side = std::clamp(distance, 0.f, 1.f);
    const float eased = side * side * (3.f - 2.f * side);
    const float tail = std::max(0.f, distance - 1.f);
    const float sign = offset < 0.f ? -1.f : 1.f;
    const float scale = 1.28f + (std::max(.62f, .94f - .075f * tail) - 1.28f) * eased;
    const float size = 300.f * scale;
    const float travel = eased * (335.f + 138.f * tail);
    const float hover = (1.f - eased) * (5.f + 2.5f * std::sin(seconds * 1.1f));
    return {center_x + sign * travel - size * .5f, floor_y - size - hover,
            size, size, -sign * .82f * eased,
            std::max(.58f, 1.f - .15f * eased - .055f * tail),
            std::max(.18f, 1.f - .10f * tail)};
}
}
