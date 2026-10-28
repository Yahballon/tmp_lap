// CPU-only pose test; PS5 SDK not needed.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../overlay/headless/prosperoeden/pe/ui/coverflow.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main() {
    using pe::ui::coverflow::pose;
    for (float cursor=0.0f; cursor<400.0f; cursor+=0.25f)
        for (int delta=-5; delta<=5; ++delta) {
            const auto p=pose(cursor+delta,cursor,1.0f);
            assert(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.w));
            assert(p.w>0.0f && p.h>0.0f);
            assert(p.opacity>0.0f && p.opacity<=1.0f);
            assert(std::fabs(p.yaw)<=1.0f && p.brightness>=0.58f);
        }
    const auto l=pose(2.0f,3.0f,0.0f), m=pose(3.0f,3.0f,0.0f),
               r=pose(4.0f,3.0f,0.0f);
    assert(m.w>l.w && m.w>r.w && l.x<m.x && r.x>m.x);
    assert(l.yaw>0.0f && r.yaw<0.0f && std::fabs(m.yaw)<0.0001f);
    // Side cases remain inside the library viewport and retain square art.
    for (int i=-3; i<=3; ++i)
    {
        const auto p=pose(10.0f+static_cast<float>(i),10.0f,0.0f);
        assert(p.w==p.h);
        assert(p.x>=180.0f);
        assert(p.x+p.w<=1300.0f);
    }
    std::cout<<"GBAXY coverflow geometry OK\n";
}
