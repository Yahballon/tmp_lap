#pragma once
// Minimal host-only stand-in for the PS5 homebrew renderer. Used exclusively
// to exercise the exact LevelPreview collision C++ without a console/GPU.
#include <cstdint>
namespace hui::gfx {
struct Color {
    static Color rgb(std::uint32_t,float=1.f){return {};}
};
struct Rect {float x,y,w,h;};
struct DrawList {
    void gradient_rect(Rect,int,Color,Color){}
    void rounded_rect(Rect,float,Color){}
    void image(unsigned,Rect,Rect,Color){}
};
struct Renderer {
    struct Batch {
        unsigned next=1;
        unsigned create_texture(int,int,const unsigned char*){return next++;}
    } textures;
    Batch& batch(){return textures;}
};
}
