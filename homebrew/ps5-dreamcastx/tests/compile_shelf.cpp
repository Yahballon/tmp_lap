// Runnable geometry/UV regression tests for the PS5 CoverFlow shell.
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
using ImU32=uint32_t;
using ImTextureID=void*;
struct ImVec2 { float x,y; ImVec2(float a=0,float b=0):x(a),y(b){} };
struct ImDrawList {
    void AddLine(ImVec2,ImVec2,ImU32,float){}
    void AddQuadFilled(ImVec2,ImVec2,ImVec2,ImVec2,ImU32){}
    void AddQuad(ImVec2,ImVec2,ImVec2,ImVec2,ImU32,float){}
    void AddImageQuad(ImTextureID,ImVec2,ImVec2,ImVec2,ImVec2,
                      ImVec2,ImVec2,ImVec2,ImVec2,ImU32){}
    void AddRectFilledMultiColor(ImVec2,ImVec2,ImU32,ImU32,ImU32,ImU32){}
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
void gradientV(ImDrawList*,float,float,float,float,ImU32,ImU32){}
void imageFill(ImDrawList*,ImTextureID,float,float,float,float,float){}
void placeholder(ImDrawList*,const Game&,float,float,float,float,float){}
void text(ImDrawList*,int,float,float,float,ImU32,const char*){}
ImTextureID coverTexture(Game&){return nullptr;}
int bold(){return 0;}
int regular(){return 0;}
ImVec2 textSize(int,int,const char*){return {100,24};}
std::string fit(int,float,const std::string& x,float){return x;}
std::string metaText(Game&){return "Dreamcast";}
struct MockDriver {float getAspectRatio(ImTextureID){return 1.f;}};
MockDriver driver;
MockDriver* imguiDriver=&driver;
#include "../src/dreamcastx_shelf.inc"
static bool near(float a,float b){return std::fabs(a-b)<0.001f;}
int main(){
    ImDrawList dl;
    lib.shelves.push_back(Shelf{});
    drawDreamcastXShelf(&dl,nullptr,false);
    const auto flat=dreamcastXFace(100,200,300,420,0);
    assert(near(flat.tl.x,100)&&near(flat.tr.x,400));
    assert(near(flat.bl.y,620)&&near(flat.br.y,620));
    const auto left=dreamcastXFace(100,200,300,420,-0.7f);
    const auto right=dreamcastXFace(100,200,300,420,0.7f);
    assert(left.tl.y>left.tr.y&&right.tr.y>right.tl.y);
    assert(left.bl.y<left.br.y&&right.br.y<right.bl.y);
    const auto mid=dreamcastXPoint(flat,0.5f,0.5f);
    assert(near(mid.x,250.f)&&near(mid.y,410.f));
    const auto image=dreamcastXRegion(flat,0.075f,0.08f,0.985f,0.99f);
    assert(image.tl.x>flat.tl.x&&image.tl.y>flat.tl.y);
    assert(image.br.x<flat.br.x&&image.br.y<flat.br.y);
    const auto sq=dreamcastXCoverUV(1.f,0.7f);
    assert(near(sq.tl.x,0.15f)&&near(sq.br.x,0.85f));
    assert(near(sq.tl.y,0.f)&&near(sq.br.y,1.f));
    const auto wide=dreamcastXCoverUV(2.f,0.7f);
    assert(wide.tl.x>sq.tl.x&&wide.br.x<sq.br.x);
    const auto tall=dreamcastXCoverUV(0.5f,0.7f);
    assert(tall.tl.y>0.f&&tall.br.y<1.f);
    const auto bad=dreamcastXCoverUV(-1,0);
    assert(std::isfinite(bad.tl.x)&&std::isfinite(bad.br.y));
}
