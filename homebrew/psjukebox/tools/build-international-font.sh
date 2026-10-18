#!/usr/bin/env bash
# PSJukebox extended European/near-Eastern Unicode SDF font, baked on CI host.
# Uses the GPL source's documented native .huifont format, no runtime font APIs.
set -euo pipefail
app="${1:?Pass PS5 app directory}"
root="$(mktemp -d)"
trap 'rm -rf "$root"' EXIT
mkdir -p "$root/tools/font-baker" "$root/src/gfx" "$root/third_party/stb" "$app/assets/fonts"
src=https://raw.githubusercontent.com/blackbearreloaded/ps5-homebrew-ui/main
curl -fsSL "$src/tools/font-baker/bake_font.cpp" -o "$root/tools/font-baker/bake_font.cpp"
curl -fsSL "$src/src/gfx/font_format.hpp" -o "$root/src/gfx/font_format.hpp"
curl -fsSL "$src/third_party/stb/stb_truetype.h" -o "$root/third_party/stb/stb_truetype.h"
curl -fsSL "$src/assets/fonts/DejaVu-LICENSE.txt" -o "$app/assets/fonts/DejaVu-LICENSE.txt"
python3 - "$root/tools/font-baker/bake_font.cpp" <<'PY'
from pathlib import Path
import sys
p=Path(sys.argv[1])
source=p.read_text()
needle='    return result;\n}\n\ntemplate <typename T>'
assert needle in source,"Upstream font baker changed: inspect before applying glyph extensions"
replacement='''    if (european)
    {
        // Music can contain names in dozens of languages and transliterations.
        // DejaVu supports much of Latin Extended, Vietnamese, Greek,
        // Cyrillic including Ukrainian, Armenian, Hebrew and basic Arabic.
        const int ranges[][2] = {
            {0x0180,0x024F}, {0x0300,0x036F},
            {0x0370,0x03FF}, {0x0400,0x052F},
            {0x0530,0x058F}, {0x0590,0x06FF},
            {0x1E00,0x1EFF}
        };
        for (const auto &range : ranges)
            for (int cp=range[0]; cp<=range[1]; ++cp)
                result.push_back(cp);
    }
    std::sort(result.begin(),result.end());
    result.erase(std::unique(result.begin(),result.end()),result.end());
    return result;
}

template <typename T>'''
p.write_text(source.replace(needle,replacement))
PY
g++ -std=c++20 -O2 -w "$root/tools/font-baker/bake_font.cpp" -o "$root/bake-font"
font=/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
test -s "$font"
"$root/bake-font" "$font" "$app/assets/fonts/international.huifont" 52 7 4096 european \
  > "$root/font-bake.log" 2>&1 || { tail -70 "$root/font-bake.log"; exit 1; }
tail -1 "$root/font-bake.log"
python3 homebrew/psjukebox/tools/verify-international-font.py "$app/assets/fonts/international.huifont"
