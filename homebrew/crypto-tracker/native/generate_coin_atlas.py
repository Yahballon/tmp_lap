#!/usr/bin/env python3
"""Offline build-time coin logo atlas from CC0 cryptocurrency-icons.
Creates one GPU-friendly RGBA sprite atlas and a C++ symbol lookup table.
No crypto-logo network requests happen on the console.
"""
from __future__ import annotations
import re
from pathlib import Path
from PIL import Image

SOURCE = Path("/tmp/cryptocurrency-icons/128/color")
ASSETS = Path("app/assets")
INCLUDE = Path("app/src/coin_atlas.inc")
TILE = 64
COLUMNS = 16
ROWS = 32
# 16*32 = 512, enough for the upstream collection.
files = sorted(p for p in SOURCE.glob("*.png")
               if re.fullmatch(r"[a-z0-9_-]{1,24}", p.stem))
assert 100 <= len(files) <= COLUMNS*ROWS, "Coin logo repository missing or unexpected size"
atlas = Image.new("RGBA", (TILE*COLUMNS,TILE*ROWS), (0,0,0,0))
symbols = []
for i,p in enumerate(files):
    try:
        with Image.open(p) as img:
            icon=img.convert("RGBA")
            icon.thumbnail((TILE-4,TILE-4),Image.Resampling.LANCZOS)
            x=(i%COLUMNS)*TILE+(TILE-icon.width)//2
            y=(i//COLUMNS)*TILE+(TILE-icon.height)//2
            atlas.alpha_composite(icon,(x,y))
        symbols.append((p.stem,i))
    except Exception as exc:
        raise RuntimeError(f"Cannot process coin logo {p}: {exc}") from exc
ASSETS.mkdir(parents=True,exist_ok=True)
(ASSETS/"coin-atlas.rgba").write_bytes(atlas.tobytes())
# Generate compiler-readable metadata and avoid runtime JSON or PNG decoding.
items=",\n".join(f'  {{"{sym}",{pos}}}' for sym,pos in symbols)
INCLUDE.write_text(
    "// Auto-generated CC0 coin icon atlas lookup. Do not edit.\n"
    "static const IconEntry kIconEntries[] = {\n" + items + "\n};\n"
    f"static constexpr unsigned kIconCount = {len(symbols)};\n",
    encoding="utf-8")
print(f"Prepared {len(symbols)} distinct coin logos; "
      f"atlas {atlas.width}x{atlas.height} RGBA")
