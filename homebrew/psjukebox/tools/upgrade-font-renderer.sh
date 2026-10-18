#!/usr/bin/env bash
# Upgrade just the PS5-DevBridge font implementation (which predates glyph
# fallback support) while preserving its other PSJukebox draw primitives.
set -euo pipefail
app="${1:?Pass PS5 app directory}"
gfx="$app/src/gfx"
src=https://raw.githubusercontent.com/blackbearreloaded/ps5-homebrew-ui/main
test -s "$gfx/draw_list.cpp"
curl -fsSL "$src/src/gfx/font.hpp" -o "$gfx/font.hpp"
curl -fsSL "$src/src/gfx/font.cpp" -o "$gfx/font.cpp"
python3 - "$gfx/draw_list.cpp" <<'PY'
from pathlib import Path
import sys
p=Path(sys.argv[1]);s=p.read_text()
start=s.index('float DrawList::text(')
end=s.index('void DrawList::push_clip(',start)
replacement='''float DrawList::text(const Font &font, std::uint32_t font_texture, std::string_view text, float x,
                     float baseline, float size, Color color, Align align, float tracking)
{
    glyph_scratch_.clear();
    const float width = font.layout(text, x, baseline, size, align, glyph_scratch_, tracking);
    // Each resolved UTF-8 glyph may come from the primary or a fallback
    // atlas. The original DevBridge code used one atlas for the whole string.
    for (const GlyphQuad &q : glyph_scratch_)
    {
        const std::uint32_t texture=q.texture!=0?q.texture:font_texture;
        const bool slotted=is_font_handle(texture);
        const float slot=slotted?static_cast<float>(texture&0xfu):0.0f;
        Instance &i=append(slotted?0u:texture);
        const Rect t=apply({q.x0,q.y0,q.x1-q.x0,q.y1-q.y0});
        set4(i.rect,t.x,t.y,t.w,t.h);
        set_color(i.color_top,color);
        set_color(i.color_bottom,color);
        set4(i.params,q.range*transform_.scale,0.0f,slot,static_cast<float>(Shape::glyph));
        set4(i.extra,q.u0,q.v0,q.u1,q.v1);
    }
    return width;
}

'''
assert 'const float range = font.sdf_range(size)' in s[start:end]
assert 'void DrawList::rotated_image' in s, "DevBridge drawing extensions changed"
s=s[:start]+replacement+s[end:]
p.write_text(s)
print("PASS updated multilingual glyph path without losing rotated_image/drawing extensions")
PY
grep -q 'add_fallback' "$gfx/font.hpp"
grep -q 'resolve(' "$gfx/font.cpp"
