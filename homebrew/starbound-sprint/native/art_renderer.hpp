// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "game.hpp"
#include "gfx/renderer.hpp"
#include <cstdint>

namespace sprint_art {
constexpr int kSpritePixels = 16;
constexpr int kAtlasPixels = 128;
bool load_atlas(hui::gfx::Renderer& renderer, std::uint32_t& texture);
bool load_backgrounds(hui::gfx::Renderer& renderer, std::uint32_t& texture);
void release_atlas(std::uint32_t& texture);
void sprite(hui::gfx::DrawList& draw, std::uint32_t texture, int index,
            hui::gfx::Rect rect, bool facing_right=true, float alpha=1.f);
void render_world(hui::gfx::DrawList& draw,const sprint::Game& game,
                  std::uint32_t texture,std::uint32_t background,int frame);
} // namespace sprint_art
