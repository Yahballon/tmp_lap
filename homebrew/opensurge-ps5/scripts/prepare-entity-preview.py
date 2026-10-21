#!/usr/bin/env python3
"""Prepare *actual* Open Surge demo-level entity positions and original art.

This only supports a documented subset of objects in the proven native preview.
SurgeScript is not executing. Unsupported types are deliberately not fabricated.
"""
from __future__ import annotations
import re
import sys
from collections import Counter
from pathlib import Path
from PIL import Image

ENTITY_RE = re.compile(r'(?m)^\s*entity\s+"([^"]{1,80})"\s+(-?\d+)\s+(-?\d+)\s+"([^"]+)"')
KNOWN = {
    "Collectible": "ring",
    "Spring Standard": "spring",
    "Spring Stronger": "spring_stronger",
    "Goal": "goal",
}
SHEETS = [
    ("images/collectibles.png", "collectibles"),
    ("images/items/springs.png", "springs"),
    ("images/items/goal.png", "goal"),
]

def convert_png_to_rgba(source: Path, target: Path) -> tuple[int, int]:
    with Image.open(source) as img:
        out = img.convert("RGBA")
        w, h = out.size
        if not 16 <= w <= 4096 or not 16 <= h <= 4096:
            raise ValueError(f"Bad source texture dimensions {source}: {(w, h)}")
        buf = bytearray(out.tobytes())
    # The original assets use magenta color-key transparency.
    for i in range(0, len(buf), 4):
        if tuple(buf[i:i+3]) == (255, 0, 255) or buf[i+3] == 0:
            buf[i:i+4] = bytes((0, 0, 0, 0))
    target.write_bytes(buf)
    return w, h

def stage(original: Path, app: Path) -> dict:
    root = original / "game"
    lines = (root / "levels/demo-1.lev").read_text(encoding="utf-8")
    matches = list(ENTITY_RE.finditer(lines))
    if len(matches) < 200:
        raise ValueError("Original demo entity list unexpectedly incomplete")
    items: list[tuple] = []
    stats: Counter = Counter()
    for m in matches:
        name, xs, ys, guid = m.groups()
        if not re.fullmatch(r"[0-9a-f]{8,32}", guid):
            raise ValueError("Unexpected original object ID")
        stats[name] += 1
        if name not in KNOWN:
            continue
        x, y = int(xs), int(ys)
        if abs(x) >= 1000000 or abs(y) >= 1000000:
            raise ValueError("Untrusted level object coordinate")
        items.append((KNOWN[name], x, y))
    if not (60 <= stats["Collectible"] <= 180 and
            stats["Goal"] == 1 and
            stats["Spring Standard"] > 0 and
            stats["Spring Stronger"] > 0):
        raise ValueError(f"Original demo objects changed: {stats}")
    dest = app / "assets" / "level-preview"
    dest.mkdir(parents=True, exist_ok=True)
    (dest / "entities.tsv").write_text(
        "".join(f"{kind}\t{x}\t{y}\n" for kind,x,y in items), encoding="ascii"
    )
    for source, stem in SHEETS:
        size = convert_png_to_rgba(root / source, dest / (stem + ".rgba"))
        (dest / (stem + ".txt")).write_text(
            f"{size[0]} {size[1]}\n", encoding="ascii"
        )
    (dest / "entity-counts.txt").write_text(
        "\n".join(f"{name}\t{stats[name]}" for name in sorted(stats)) + "\n",
        encoding="utf-8"
    )
    result = {"all": len(matches), "supported": len(items),
              "rings": stats["Collectible"],
              "springs": stats["Spring Standard"] + stats["Spring Stronger"],
              "goals": stats["Goal"]}
    print("ORIGINAL_OPEN_SURGE_ENTITIES",result)
    return result

if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("Usage: prepare-entity-preview.py <staged-assets> <app>")
    stage(Path(sys.argv[1]), Path(sys.argv[2]))
