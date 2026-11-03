// AndroidFlow by YAHBALLON — testable native Godot cursor navigation.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef YAH_GODOT_CURSOR_H
#define YAH_GODOT_CURSOR_H
#include <math.h>
typedef struct { float x, y, dx, dy; } yah_godot_cursor;
static inline float yah_pointer_clamp(float x, float lo, float hi) {
    return x < lo ? lo : x > hi ? hi : x;
}
// Coordinates are native Android viewport pixels, top-left origin (1920x1080).
// No movement while centred; speed is frame-rate independent.
static inline int yah_pointer_tick(yah_godot_cursor *p, float dt) {
    const float deadzone = 0.15f, speed = 1550.f;
    const float m = sqrtf(p->dx * p->dx + p->dy * p->dy);
    if (m <= deadzone || dt <= 0.f) return 0;
    const float a = yah_pointer_clamp((m - deadzone) / (1.f - deadzone), 0.f, 1.f);
    const float step = speed * a * a * yah_pointer_clamp(dt, 0.f, 0.05f);
    const float x = yah_pointer_clamp(p->x + p->dx / m * step, 0.f, 1919.f);
    const float y = yah_pointer_clamp(p->y + p->dy / m * step, 0.f, 1079.f);
    int moved = x != p->x || y != p->y;
    p->x = x;
    p->y = y;
    return moved;
}
static inline int yah_pointer_coord(float normalized, int dimension) {
    return (int)(yah_pointer_clamp(normalized, 0.f, 1.f) * (dimension - 1) + 0.5f);
}
#endif
