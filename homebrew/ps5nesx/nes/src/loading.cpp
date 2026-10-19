// NESYX: neon-blue and purple native loading screen with real scan progress.
#include "loading.h"
#include "ps5nes_build_version.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace nes {
namespace {
constexpr uint32_t Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a=255) {
    return uint32_t(r) | (uint32_t(g)<<8) | (uint32_t(b)<<16) | (uint32_t(a)<<24);
}
} // namespace

void BuildLoadingFrame(fe::FrameDesc& frame, const fe::Fonts& fonts, unsigned width,
                       unsigned height, const ScanProgress& progress, double elapsed, bool done) {
    frame=fe::FrameDesc{};
    const float W=static_cast<float>(width), H=static_cast<float>(height), k=H/2160.0f;
    // The native background shader supplies the same dark synthwave glow as the shelf.
    const float top[4]={0.010f,0.014f,0.050f,1.0f};
    const float mid[4]={0.027f,0.009f,0.082f,1.0f};
    const float bottom[4]={0.008f,0.007f,0.035f,1.0f};
    std::copy(top,top+4,frame.bg.top);
    std::copy(mid,mid+4,frame.bg.mid);
    std::copy(bottom,bottom+4,frame.bg.bottom);
    frame.bg.glow_color[0]=0.08f;
    frame.bg.glow_color[1]=0.49f;
    frame.bg.glow_color[2]=1.0f;
    frame.bg.glow_color[3]=0.42f;
    frame.bg.glow_pos[0]=0.29f;
    frame.bg.glow_pos[1]=0.46f;
    frame.bg.glow_pos[2]=0.70f;
    frame.bg.glow_pos[3]=0.55f;
    frame.bg.floor_glow[0]=0.79f;
    frame.bg.floor_glow[1]=0.13f;
    frame.bg.floor_glow[2]=0.99f;
    frame.bg.floor_glow[3]=0.07f;
    frame.bg.misc[0]=W/H;
    frame.bg.misc[1]=0.8f;
    frame.bg.misc[2]=0.55f;
    frame.bg.misc[3]=static_cast<float>(elapsed);

    auto& ui=frame.ui;
    const uint32_t white=Color(247,248,255),pale=Color(183,205,244),cyan=Color(49,219,255);
    const uint32_t pink=Color(250,70,224),panel=Color(8,16,48,235);
    auto rect=[&](float x,float y,float w,float h,float radius,uint32_t color) {
        fe::Fonts::AddRoundedRect(ui,x,y,w,h,radius,color);
    };
    rect(0,0,W,5*k,0,cyan);
    rect(0,5*k,W*0.5f,3*k,0,Color(0,113,250,200));
    rect(W*0.5f,5*k,W*0.5f,3*k,0,pink);
    for(int i=1;i<=14;++i)
        rect(0,H*0.73f+i*(H*0.018f),W,1.8f*k,0,Color(35,108,235,26));
    const float left=104*k;
    rect(left,98*k,9*k,100*k,4*k,cyan);
    fonts.AddText(ui,"NESYX",left+42*k,166*k,83*k,white,0.72f);
    const std::string credit=std::string("by YAHBALLON  |  v")+nes_build::kVersion;
    fonts.AddText(ui,credit.c_str(),left+45*k,220*k,34*k,Color(182,185,255),0.25f);

    const float panelW=std::min(W-220*k,2280*k),panelX=(W-panelW)*0.5f;
    const float panelY=H*0.285f,panelH=H*0.45f;
    rect(panelX-3*k,panelY-3*k,panelW+6*k,panelH+6*k,40*k,Color(41,191,255,135));
    rect(panelX,panelY,panelW,panelH,36*k,panel);
    rect(panelX+36*k,panelY+38*k,76*k,6*k,3*k,cyan);
    rect(panelX+119*k,panelY+38*k,42*k,6*k,3*k,pink);
    fonts.AddText(ui,done?"Games ready":"Loading games",
                  W*0.5f,panelY+205*k,96*k,white,0.62f,fe::Fonts::Center);
    const std::string counts=std::to_string(progress.games)+" games found  |  "+
                             std::to_string(progress.entries)+" entries read";
    fonts.AddText(ui,counts.c_str(),W*0.5f,panelY+303*k,45*k,pale,0.22f,fe::Fonts::Center);

    const float barX=panelX+110*k,barW=panelW-220*k,barY=panelY+369*k,barH=25*k;
    rect(barX,barY,barW,barH,12*k,Color(41,59,112));
    // NFS may not know total files; use an animated pulse until folder count is known.
    const bool measured=done||progress.sources_total>0;
    const float ratio=done?1.0f:measured?
        std::clamp(float(progress.sources_done)/float(std::max(1,progress.sources_total)),0.0f,1.0f):0.0f;
    if(measured && ratio>0.0f) {
        rect(barX,barY,std::max(16*k,barW*ratio),barH,12*k,cyan);
        if(ratio>0.45f)
            rect(barX+barW*ratio*0.53f,barY,barW*ratio*0.47f,barH,12*k,pink);
    } else if(!done) {
        const float pulse=std::sin(static_cast<float>(elapsed)*2.8f)*0.5f+0.5f;
        const float pulseW=std::min(barW,240*k);
        rect(barX+(barW-pulseW)*pulse,barY,pulseW,barH,12*k,cyan);
    }
    std::string location=progress.directory.empty()?"Preparing library...":progress.directory;
    if(location.size()>110)location="..."+location.substr(location.size()-107);
    fonts.AddText(ui,location.c_str(),W*0.5f,panelY+515*k,34*k,pale,0.12f,fe::Fonts::Center);
    const std::string folders="Folders checked: "+std::to_string(progress.sources_done)+" / "+
                              std::to_string(progress.sources_total);
    fonts.AddText(ui,folders.c_str(),W*0.5f,panelY+604*k,35*k,Color(184,160,235),0.14f,fe::Fonts::Center);
    rect(panelX+40*k,panelY+panelH-14*k,(panelW-80*k)*0.5f,5*k,2*k,Color(50,172,255,150));
    rect(W*0.5f,panelY+panelH-14*k,(panelW-80*k)*0.5f,5*k,2*k,Color(217,75,247,150));
}
} // namespace nes
