// Port adapter for the ORIGINAL Project Porpoise Gfx renderer, shaders,
// perspective, cover-flow arrangement and liquid-glass card primitives.
// Copyright (C) 2026 Ruben (Project Porpoise), GPL-3.0-or-later.
// Adaptation for NES game selection: YAHBALLON, GPL-3.0-or-later.
#include "fe_exact_porpoise.h"
#include "../porpoise_original/src/ui_app_common.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>
namespace fe {
using namespace porpoise::ui;
using namespace porpoise::ui::look;
using PorpoiseTexture = porpoise::ui::Texture;

bool ExactPorpoiseShelf::Init(const GfxInit& config, const std::vector<GameInfo>& games)
{
    m_names.clear();
    for (const GameInfo& g : games) m_names.push_back(g.title);
    m_art.assign(games.size(), nullptr);
    std::printf("[Porpoise Exact] texture_uv=flat_quad_rect+v_local (v1.0.18)\n");
    m_ready = m_gfx.init(config);
    std::printf("[Porpoise Exact] original UI Gfx init=%d, games=%zu\n",
                m_ready ? 1 : 0, games.size());
    return m_ready;
}
void ExactPorpoiseShelf::Shutdown()
{
    if (!m_ready) return;
    for (PorpoiseTexture* t : m_art) if (t) m_gfx.free_texture(t);
    m_art.clear();
    m_gfx.shutdown();
    m_ready = false;
}
void ExactPorpoiseShelf::Cover(const CoverImage& image)
{
    if (!m_ready || image.kind != CoverImage::Cover || image.game < 0 ||
        static_cast<size_t>(image.game) >= m_art.size() ||
        image.width <= 0 || image.height <= 0 ||
        image.rgba.size() != static_cast<size_t>(image.width) * image.height * 4)
        return;
    PorpoiseTexture*& art = m_art[static_cast<size_t>(image.game)];
    PorpoiseTexture* replacement = m_gfx.texture_rgba(image.rgba.data(), image.width, image.height);
    if (replacement) {
        if (art) m_gfx.free_texture(art);
        art = replacement;
    }
}
void ExactPorpoiseShelf::Select(int index, float scroll, double elapsed, const std::string& clock)
{
    if (index != m_selected || m_recent.empty())
    {
        m_recent.erase(std::remove(m_recent.begin(), m_recent.end(), index), m_recent.end());
        m_recent.insert(m_recent.begin(), index);
        if (m_recent.size() > 3) m_recent.resize(3);
    }
    m_selected = index;
    m_scroll = scroll;
    m_elapsed = elapsed;
    m_clock = clock;
}

// Exactly the Project Porpoise front-cover rendering method, adapted only
// to get artwork from the NES library instead of Dolphin's Library::Game.
void ExactPorpoiseShelf::DrawTile(int i, float cx, float cy, float w, float h,
                                  float yaw, float alpha, bool focused, bool reflection)
{
    Gfx& g = m_gfx;
    const float m = 13.0f; /* glass frame margin */
    const float glow = focused ? 20.0f : 0.0f;
    const float inner_r = kR * 0.6f;
    PorpoiseTexture* cover = i >= 0 && static_cast<size_t>(i) < m_art.size() ? m_art[static_cast<size_t>(i)] : nullptr;
    if (reflection)
    {
        const float base = cy + h * 0.5f + m;
        const Corner r[4] = {project(cx, base, -w * 0.5f, 0, yaw),
                             project(cx, base, w * 0.5f, 0, yaw),
                             project(cx, base, w * 0.5f, h * 0.85f, yaw),
                             project(cx, base, -w * 0.5f, h * 0.85f, yaw)};
        if (cover) {
            float uv[4] = {0, 0, 1, 1};
            const float ta = float(cover->width) / float(cover->height), aa = w / h;
            if (ta > aa) {
                const float k = aa / ta;
                uv[0] = 0.5f - k * 0.5f;
                uv[2] = 0.5f + k * 0.5f;
            }
            uv[1] = 0.15f;
            g.quad3d(cover, r, w, h * 0.85f, with_alpha(rgba(0xB8C8FF), 0.30f * alpha),
                     inner_r, true, true, uv);
        } else {
            g.quad3d(nullptr, r, w, h * 0.85f,
                     with_alpha(rgba(0x2A4FD0), 0.25f * alpha), inner_r, true, true);
        }
        return;
    }
    Glass face;
    face.tint = kGlassBody;
    face.rim = focused ? kIcy : kEdge;
    face.fade = alpha;
    face.radius = kR;
    face.rim_w = focused ? 3.2f : 2.2f;
    face.glow = glow;
    face.phase = cx / 1920.0f;
    const float depth = 34.0f * (w / kTileW);
    glass_block(g, cx, cy, w + m * 2, h + m * 2, depth, yaw, 0, glow * 3, face);
    const Corner c[4] = {project(cx, cy, -w * 0.5f, -h * 0.5f, yaw),
                         project(cx, cy, w * 0.5f, -h * 0.5f, yaw),
                         project(cx, cy, w * 0.5f, h * 0.5f, yaw),
                         project(cx, cy, -w * 0.5f, h * 0.5f, yaw)};
    if (cover)
    {
        float uv[4] = {0, 0, 1, 1};
        const float ta = float(cover->width) / float(cover->height), aa = w / h;
        if (ta > aa) {
            const float k = aa / ta;
            uv[0] = 0.5f - k * 0.5f;
            uv[2] = 0.5f + k * 0.5f;
        } else {
            const float k = ta / aa;
            uv[1] = 0.5f - k * 0.5f;
            uv[3] = 0.5f + k * 0.5f;
        }
        g.quad3d(cover, c, w, h, with_alpha(kWhite, alpha), inner_r,
                 false, false, uv);
    } else {
        g.quad3d(nullptr, c, w, h, with_alpha(rgba(0x16328F), alpha),
                 inner_r, false, false);
    }
    Glass gloss;
    gloss.face = 2;
    gloss.radius = inner_r;
    gloss.rim_w = 0;
    gloss.tint = kWhite;
    gloss.rim = kClear;
    gloss.fade = alpha;
    gloss.phase = cx / 1920.0f;
    g.glass(c, w, h, 0, gloss);
}
void ExactPorpoiseShelf::SetTheme(const std::string& requested)
{
    std::string next=requested;
    if(next!="porpoise" && next!="oled" && next!="cube" &&
       next!="aurora" && next!="synthwave") next="porpoise";
    if(next==m_theme)return;
    m_theme=next;
    std::printf("[Porpoise Exact] theme=%s\n",m_theme.c_str());
}

void ExactPorpoiseShelf::Prepare(unsigned slot, float w, float h)
{
    if (!m_ready) return;
    Gfx& g = m_gfx;
    // Source: Porpoise make_porpoise(), ui_app_common.hpp and
    // ui_app.cpp (the top bar, library and dock visual values).
    // Original Project Porpoise look parameters:
    // source porpoise_original/src/ui_theme.cpp (make_* methods).
    // We deliberately preserve its existing Vulkan shader/backgrounds.
    Look look;
    look.panel_style = 1;
    float radius=24.0f;
    if(m_theme=="oled")
    {
        look.background=1;look.dark=0.12f;look.fill_alpha=1.5f;
        look.saturation=0.9f;look.flat=true;radius=22.0f;
    }
    else if(m_theme=="cube")
    {
        look.background=3;
        look.light0={0.42f,0.22f,0.95f,1.0f};
        look.light1={0.66f,0.48f,1.0f,1.0f};
        look.hue=0.735f;look.hue_spread=0.35f;radius=20.0f;
    }
    else if(m_theme=="aurora")
    {
        look.background=7;
        look.light0={0.15f,0.95f,0.62f,1.0f};
        look.light1={0.55f,0.35f,1.0f,1.0f};
        look.hue=0.50f;look.hue_spread=0.7f;
    }
    else if(m_theme=="synthwave")
    {
        look.background=10;
        look.light0={1.0f,0.22f,0.78f,1.0f};
        look.light1={0.20f,0.85f,1.0f,1.0f};
        look.hue=0.86f;look.hue_spread=0.7f;radius=18.0f;
    }
    kR=radius;
    g.begin(slot, w, h, static_cast<float>(m_elapsed), 0, false);
    g.set_look(look);
    g.background();

    // Compact header requested for PS5NESX: no Library / Save States /
    // Settings tabs and no L1/R1 glyphs. Square below opens the real panel.
    if (g.brand_mask()) g.image(g.brand_mask(), 48, kBarCy-32, 64, 64, kCyan);
    g.text_mid(Font::SemiBold, 30, 132, kBarCy, rgba(0xBFE9FF), Align::Left,
               "PS5NES", 7.0f);
    if(!m_clock.empty())
        g.text_mid(Font::SemiBold,27,1866,kBarCy,kWhite,Align::Right,m_clock);

    // Compact, non-blocking progress HUD. The data comes from the cover
    // worker's mutex-protected snapshot; rendering never touches networking.
    const CoverProgress& progress = m_cover_progress;
    // The status box is transient: it disappears automatically when the
    // final missing ROM has been checked (or when the worker stops because
    // the network is unavailable). A manual R2 refresh starts a new cycle.
    if(progress.games > 0 && progress.planned > 0 &&
       !progress.done && !progress.offline)
    {
        constexpr float px=1414,py=149,pw=452,ph=100;
        g.panel(px,py,pw,ph,rgba(0x061339,0.78f),0.84f,19,
                rgba(0x42AFFF,0.78f),1.3f,0,0.15f);
        std::string state;
        if(progress.transferring)
        {
            state = "Fetching: " + progress.current;
            if(state.size()>42) state=state.substr(0,39)+"...";
        }
        else state="Searching for missing game covers";
        g.text(Font::SemiBold,19,px+19,py+14,kWhite,Align::Left,state);
        char detail[100];
        std::snprintf(detail,sizeof(detail),"%d / %d checked   |   %d saved   |   %d unavailable",
            progress.attempted,progress.planned,progress.downloaded,progress.failed);
        g.text(Font::SemiBold,16,px+19,py+45,kLavender,Align::Left,detail);
        const float barX=px+19,barY=py+80,barW=pw-38;
        g.panel(barX,barY,barW,6,rgba(0x25426B,0.78f),0.85f,3);
        const float fraction=std::clamp(progress.planned > 0 ?
            float(progress.attempted)/float(progress.planned) : 0.0f,0.0f,1.0f);
        if(fraction>0)
            g.panel(barX,barY,std::max(3.0f,barW*fraction),6,rgba(0x60D7FF),0.95f,3);
        if(progress.transferring && fraction<1)
        {
            // Moving pulse means the current request is still active even if
            // its download progress has no reported byte count.
            const float remaining=barW*(1.0f-fraction);
            const float pulse=std::fmod(float(m_elapsed)*0.45f,1.0f);
            g.panel(barX+barW*fraction+pulse*std::max(1.0f,remaining-18),
                    barY,18,6,rgba(0xBCEEFF),0.9f,3);
        }
    }

    const int shown=static_cast<int>(m_names.size());
    g.text(Font::Bold,54,90,132,kWhite,Align::Left,"Your Games");
    char counter[48];
    std::snprintf(counter,sizeof counter,"%d games",shown);
    g.text(Font::SemiBold,32,92,200,kLavender,Align::Left,counter);

    // Original drawing order, offsets, widths, scales and yaw.
    std::vector<int> order;
    for(int i=0;i<shown;++i)
        if(std::fabs(float(i)-m_scroll)<4.2f) order.push_back(i);
    std::sort(order.begin(),order.end(),[&](int a,int b){
        return std::fabs(float(a)-m_scroll)>std::fabs(float(b)-m_scroll);
    });
    for(int pass=0;pass<2;++pass)
        for(int i:order)
        {
            const float k=float(i)-m_scroll;
            const float a=std::fabs(k),s=k<0?-1.0f:1.0f;
            float x,scale,yaw;
            if(a<=1.0f)
            {
                x=kCx+s*a*372.0f;
                scale=1.0f-0.14f*a;
                yaw=s*a*0.22f;
            }
            else
            {
                x=kCx+s*(372.0f+(a-1.0f)*284.0f);
                scale=std::max(0.5f,0.86f-0.10f*(a-1.0f));
                yaw=s*0.22f;
            }
            const float alpha=std::clamp(1.0f-(a-3.0f),0.0f,1.0f);
            const bool focused=i==m_selected && a<0.5f;
            const float sway=std::sin(float(m_elapsed)*0.9f)*0.06f*
                (1.0f-std::min(a*2.0f,1.0f));
            DrawTile(i,x,kCy,kTileW*scale,kTileH*scale,yaw+sway,alpha,focused,pass==0);
        }
    if(m_selected>0) g.glyph(Glyph::Arrow,34,kCy,46,rgba(0x58B8FF),-kPi*0.5f);
    if(m_selected+1<shown) g.glyph(Glyph::Arrow,1886,kCy,46,rgba(0x58B8FF),kPi*0.5f);
    if(m_selected>=0 && m_selected<shown)
    {
        g.text(Font::Bold,46,kCx,748,kWhite,Align::Center,m_names[m_selected]);
        g.text(Font::SemiBold,28,kCx,812,kLavender,Align::Center,
               "Nintendo Entertainment System");
    }

    // Porpoise bottom glass dock: show ACTUALLY visited selections.
    // This is 'Recently Browsed', not a fabricated play-history list.
    const int count=static_cast<int>(m_recent.size());
    if(count>0)
    {
        constexpr float ch=74,cw=ch*5.0f/7.0f,gap=18,label_w=290,dh=96,dy=902;
        const float dw=32+label_w+count*(cw+gap)-gap+32,dx=kCx-dw*0.5f;
        g.panel(dx,dy,dw,dh,rgba(0x0F1F63,0.55f),0.75f,kR,
                rgba(0x4C6FD8,0.55f),1.6f,0,0.12f);
        g.text(Font::SemiBold,22,dx+32,dy+22,kLavender,Align::Left,"Recently Browsed");
        const int last=std::clamp(m_recent[0],0,std::max(0,shown-1));
        if(shown>0)
            g.text(Font::Bold,24,dx+32,dy+54,kSoft,Align::Left,m_names[last]);
        for(int j=0;j<count;++j)
        {
            const int idx=m_recent[j];
            if(idx<0||idx>=shown) continue;
            const float cx=dx+32+label_w+j*(cw+gap)+cw*0.5f;
            const float cy=dy+dh*0.5f;
            PorpoiseTexture* cover=m_art[static_cast<size_t>(idx)];
            if(cover)
            {
                float uv[4];
                cover_uv(cover,cw,ch,uv);
                g.image_part(cover,cx-cw*0.5f,cy-ch*0.5f,cw,ch,uv,kWhite,5);
            }
            else
                g.panel(cx-cw*0.5f,cy-ch*0.5f,cw,ch,rgba(0x1E46C8,0.55f),
                        0.7f,5,rgba(0x4C8DFF,0.8f),1.2f);
        }
    }

    // Same prompt alignment and icon glyph renderer as Porpoise.
    constexpr float hintY=1010;
    struct Prompt {Glyph glyph; const char* text;};
    const Prompt left[]={{Glyph::DPad,"Browse"},{Glyph::Cross,"Play"}};
    const Prompt right[]={{Glyph::Square,"Settings"}};
    float lx=58;
    for(const auto& p:left)
    {
        g.glyph(p.glyph,lx+22,hintY,44,kWhite);
        lx+=52+g.text_mid(Font::SemiBold,24,lx+52,hintY,kWhite,Align::Left,p.text)+40;
    }
    float rx=1862;
    for(int i=0;i>=0;--i)
    {
        const float tw=g.measure(Font::SemiBold,24,right[i].text);
        g.text_mid(Font::SemiBold,24,rx,hintY,kWhite,Align::Right,right[i].text);
        g.glyph(right[i].glyph,rx-tw-26,hintY,44,kWhite);
        rx-=tw+98;
    }
    char index_text[32];
    std::snprintf(index_text,sizeof index_text,"%02d / %02d",m_selected+1,shown);
    g.text_mid(Font::SemiBold,24,960,hintY,kLavender,Align::Center,index_text,2);
}
} // namespace fe
