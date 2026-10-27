#include "../coverflow_layout.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main() {
    using pe::ui::gbaxy::pose;
    for(float index=0;index<400;index+=0.25f) {
        for(int offset=-5;offset<=5;offset++) {
            auto p=pose(index+offset,index,3.5f);
            assert(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.width));
            assert(p.width>0&&p.height>0&&p.opacity>0&&p.opacity<=1);
            assert(std::fabs(p.yaw)<=1.f&&p.brightness>=0.58f);
        }
    }
    auto l=pose(2,3,0),m=pose(3,3,0),r=pose(4,3,0);
    assert(l.x<m.x&&r.x>m.x&&m.width>l.width&&m.width>r.width);
    assert(l.yaw>0&&r.yaw<0&&std::fabs(m.yaw)<.0001f);
    std::cout<<"coverflow geometry OK\n";
}
