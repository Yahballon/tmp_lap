// GBAXY-style native coverflow poses adapted to ProsperoEden's 1920x1080 UI.
// Based on Yahballon/PS5GBAX ps5/frontend/fe_coverflow_layout.h.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>
namespace pe::ui::coverflow {
struct Pose { float x, y, w, h, yaw, brightness, opacity; };
inline Pose pose(float index, float scroll, float time,
                 float center_x=785.0f, float floor_y=692.0f) {
    const float delta=index-scroll, distance=std::fabs(delta);
    const float sign=delta<0.0f?-1.0f:1.0f;
    const float n=std::clamp(distance,0.0f,1.0f);
    const float side=n*n*(3.0f-2.0f*n);
    const float tail=std::max(0.0f,distance-1.0f);
    const float scale=1.28f+(std::max(0.62f,0.94f-0.075f*tail)-1.28f)*side;
    const float extent=300.0f*scale;
    const float x=center_x+sign*side*(335.0f+138.0f*tail)-extent*0.5f;
    const float y=floor_y-extent-(1.0f-side)*(5.0f+2.5f*std::sin(time*1.1f));
    return {x,y,extent,extent,-sign*0.82f*side,
            std::max(0.58f,1.0f-0.15f*side-0.055f*tail),
            std::max(0.18f,1.0f-0.10f*tail)};
}
}
