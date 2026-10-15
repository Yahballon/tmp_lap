// Host syntax-only test doubles; not a PS5 rendering benchmark.
#include <algorithm>
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
 void AddImageQuad(ImTextureID,ImVec2,ImVec2,ImVec2,ImVec2,ImVec2,ImVec2,ImVec2,ImVec2,ImU32){}
};
namespace col{
 const ImU32 accent=1,dim=2,text=3;
 ImU32 rgba(int,int,int,int){return 0;}
}
struct Game{std::string title;};
struct Shelf{std::vector<size_t>items;int focus=0;float focusAnim=-1;std::string name;};
struct Library{std::vector<Shelf>shelves;int shelf=0;bool details=false;};
static Library lib;
static std::vector<Game> games;
static const float W=1920,H=1080,S=1;
static const double timeNow=0;
enum Motion{MotionOff,MotionOn};
Motion motion(){return MotionOn;}
ImVec2 V(float x,float y){return {x,y};}
ImU32 alpha(ImU32 c,float){return c;}
float approach(float,float y,float){return y;}
void softDisc(ImDrawList*,float,float,float,ImU32,int){}
void gradientV(ImDrawList*,float,float,float,float,ImU32,ImU32){}
void quad(ImDrawList*,ImVec2,ImVec2,ImVec2,ImVec2,ImU32,ImU32,ImU32,ImU32){}
void glow(ImDrawList*,float,float,float,float,float,ImU32,int){}
ImTextureID coverTexture(Game&){return nullptr;}
void drawCover(ImDrawList*,Game&,float,float,float,float,float,float){}
int bold(){return 0;}
int regular(){return 0;}
ImVec2 textSize(int,int,const char*){return {100,24};}
void text(ImDrawList*,int,int,float,float,ImU32,const char*){}
std::string fit(int,int,const std::string& x,float){return x;}
std::string metaText(Game&){return "Dreamcast";}
#include "../src/dreamcastx_shelf.inc"
int main(){lib.shelves.push_back(Shelf{});ImDrawList dl;drawDreamcastXShelf(&dl,nullptr,false);}
