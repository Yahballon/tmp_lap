// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "gfx/renderer.hpp"
#include "market_client.hpp"
#include <cstdint>
#include <string_view>
namespace tracker {
// Loads a single offline RGBA atlas and uses unique fallback monograms
// when a coin logo is absent from the bundled CC0 collection.
class CoinLogos {
public:
    bool init(hui::gfx::Renderer& renderer);
    void draw(hui::gfx::DrawList& ui,
              const hui::gfx::Font& font,std::uint32_t fontAtlas,
              const Coin& coin,float x,float y,float diameter=58.0f) const;
private:
    std::uint32_t texture_ = 0;
    int index_of(std::string_view symbol) const;
};
} // namespace tracker
