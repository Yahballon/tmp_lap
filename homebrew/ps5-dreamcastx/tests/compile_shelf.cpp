// Runnable host-side regression tests for DVD case artwork fitting.
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
using ImU32=uint32_t;
using ImTextureID=void*;
struct ImVec2{float x,y;};
struct ImDrawList{
    void AddLine(ImVec2,ImVec2,ImU32,float){}
    void PushClipRect(ImVec2,ImVec2,bool){}
    void PopClipRect(){}
};
namespace col { constexpr ImU32 accent=1,dim=2,text=3; ImU32 rgba(int,int,int,int){return 0;} }
struct Game{std::string title;};
struct Shelf{std::vector<size_t>items;int focus=0;float focusAnim=-1;std::string name;};
struct Library{std::vector<Shelf>shelves;int shelf=0;bool details=false;};
static Library lib;
static std::vector<Game> games;
static constexpr float W=1920,H=1080,S=1;
enum Motion{MotionOff,MotionOn};
Motion motion(){return MotionOn;}
ImVec2 V(float x,float y){return {x,y};}
ImU32 alpha(ImU32 c,float){return c;}
float approach(float,float y,float){return y;}
void rect(ImDrawList*,float,float,float,float,ImU32,float=0.f){}
void outline(ImDrawList*,float,float,float,float,ImU32,float,float){}
void imageFill(ImDrawList*,ImTextureID,float,float,float,float,float){}
void placeholder(ImDrawList*,const Game&,float,float,float,float,float){}
void text(ImDrawList*,int,float,float,float,ImU32,const char*){}
ImTextureID coverTexture(Game&){return nullptr;}
int bold(){return 0;}
int regular(){return 0;}
ImVec2 textSize(int,int,const char*){return {100,24};}
std::string fit(int,float,const std::string& x,float){return x;}
std::string metaText(Game&){return "Dreamcast";}
struct MockDriver{float getAspectRatio(ImTextureID){return 1.f;}};
MockDriver driver;
MockDriver* imguiDriver=&driver;
#include "../src/dreamcastx_shelf.inc"
int main(){
    ImDrawList dl;
    lib.shelves.push_back(Shelf{});
    drawDreamcastXShelf(&dl,nullptr,false);
    const auto sq=dreamcastXFit(0,0,240,300,1);
    assert(std::fabs(sq.x)<0.01f&&std::fabs(sq.y-30)<0.01f);
    assert(std::fabs(sq.w-240)<0.01f&&std::fabs(sq.h-240)<0.01f);
    const auto pt=dreamcastXFit(10,20,240,320,0.7f);
    assert(pt.x>=10 && pt.y>=20 && pt.x+pt.w<=250.01f && pt.y+pt.h<=340.01f);
    assert(std::fabs(pt.w/pt.h-0.7f)<0.01f);
    const auto ls=dreamcastXFit(0,0,240,320,2.0f);
    assert(std::fabs(ls.w-240)<0.01f && std::fabs(ls.h-120)<0.01f);
    const auto invalid=dreamcastXFit(0,0,240,320,-1);
    assert(std::isfinite(invalid.w)&&std::isfinite(invalid.h));
}
