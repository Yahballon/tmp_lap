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
void ExactPorpoiseShelf::Prepare(unsigned slot, float w, float h)
{
    if (!m_ready) return;
    Gfx& g = m_gfx;
    Look look;
    look.background = 0;   // original sapphire grid room
    look.panel_style = 1;  // original liquid-glass theme
    g.begin(slot, w, h, static_cast<float>(m_elapsed), 0, false);
    g.set_look(look);
    g.background();

    g.text(Font::Bold, 54, 90, 132, kWhite, Align::Left, "Your Games");
    char counter[64];
    std::snprintf(counter, sizeof counter, "%zu games", m_names.size());
    g.text(Font::SemiBold, 32, 92, 200, kLavender, Align::Left, counter);
    g.text_mid(Font::SemiBold, 26, 1866, kBarCy, kWhite, Align::Right, m_clock);
    const int shown = static_cast<int>(m_names.size());
    std::vector<int> order;
    for(int i=0; i<shown; ++i)
        if(std::fabs(float(i)-m_scroll) < 4.2f) order.push_back(i);
    std::sort(order.begin(), order.end(),
              [&](int a,int b) { return std::fabs(float(a)-m_scroll) > std::fabs(float(b)-m_scroll); });
    for(int pass=0; pass<2; ++pass)
        for(int i: order)
        {
            const float k=float(i)-m_scroll;
            const float a=std::fabs(k), s=k<0?-1.0f:1.0f;
            float x, scale, yaw;
            // ORIGINAL Porpoise ui_app_views.cpp layout() constants.
            if (a <= 1.0f) {
                x=kCx+s*a*372.0f;
                scale=1.0f-0.14f*a;
                yaw=s*a*0.22f;
            } else {
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
    if(m_selected>=0 && m_selected<shown)
    {
        g.text(Font::Bold, 46, kCx, 748.0f, kWhite,
               Align::Center, m_names[static_cast<size_t>(m_selected)]);
        g.text(Font::SemiBold, 28, kCx, 812.0f, kLavender,
               Align::Center, "Nintendo Entertainment System");
        char pos[32]; std::snprintf(pos,sizeof pos,"%02d / %02d",m_selected+1,shown);
        g.text(Font::SemiBold, 26, 960, 1010, kWhite, Align::Center, pos);
    }
    g.text(Font::SemiBold, 24, 90, 1010, kSoft, Align::Left,
           "D-Pad  Browse    X  Play    Square  Settings");
}
} // namespace fe
