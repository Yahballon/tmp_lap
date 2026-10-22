#!/usr/bin/env python3
"""Turn ORIGINAL Open Surge level and brick definitions into a bounded PS5 render cache.

The cache is generated from upstream v0.6.1.3 and is not a substitute for the
SurgeScript VM, physics engine or Allegro renderer. No game logic is fabricated.
"""
from __future__ import annotations
import re
import sys
from pathlib import Path
from PIL import Image
from collections import Counter

BRICK_HEAD = re.compile(r"(?m)^\s*brick\s+(\d+)\s*\{")
BRICK_INSTANCE = re.compile(r"^\s*brick\s+(\d+)\s+(-?\d+)\s+(-?\d+)(?:\s+(green|yellow))?\s*$")
DEF_TYPE = re.compile(r"\btype\s+(PASSABLE|CLOUD|SOLID)\b")
RECT = re.compile(r"\bsource_rect\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)")
SIZE = re.compile(r"\bframe_size\s+(\d+)\s+(\d+)")
INDEX = re.compile(r"\bzindex\s+(-?[0-9.]+)")
SPAWN = re.compile(r'(?m)^\s*spawn_point\s+(-?\d+)\s+(-?\d+)\s*$')
THEME = re.compile(r'(?m)^\s*theme\s+"([^"]+)"')


def blocks(text: str):
    for head in BRICK_HEAD.finditer(text):
        depth, end = 1, head.end()
        while end < len(text) and depth:
            if text[end] == "{":
                depth += 1
            elif text[end] == "}":
                depth -= 1
            end += 1
        if depth:
            raise ValueError(f"Unmatched brick block {head[1]}")
        yield int(head[1]), text[head.end():end - 1]


def run(staged: Path, app: Path) -> dict:
    root = staged / "game"
    level = root / "levels" / "demo-1.lev"
    assert level.is_file() and (root / "images/players/surge.png").is_file()
    text = level.read_text(encoding="utf-8")
    spawn = SPAWN.search(text)
    theme = THEME.search(text)
    if not spawn or not theme:
        raise ValueError("Missing Open Surge level spawn or theme")
    theme_path = root / theme[1]
    if theme_path.resolve().is_relative_to(root.resolve()) is False:
        raise ValueError("Unsafe theme file name")
    definition = theme_path.read_text(encoding="utf-8")
    defs: dict[int, tuple] = {}
    for ident, block in blocks(definition):
        match = RECT.search(block)
        size = SIZE.search(block)
        typ = DEF_TYPE.search(block)
        if not (match and size and typ):
            raise ValueError(f"Missing atlas properties of original brick {ident}")
        sx, sy, sheetw, sheeth = map(int, match.groups())
        width, height = map(int, size.groups())
        z = INDEX.search(block)
        if not 0 < width <= sheetw <= 2048 or not 0 < height <= sheeth <= 2048:
            raise ValueError(f"Bad source rectangle for brick {ident}")
        layer = float(z[1]) if z else 0.5
        if not -2 <= layer <= 2:
            raise ValueError("Invalid layer order")
        if ident in defs:
            raise ValueError("Duplicate brick definition")
        defs[ident] = (sx, sy, width, height, typ[1], layer)

    items = []
    for line in text.splitlines():
        match = BRICK_INSTANCE.match(line)
        if match:
            ident, x, y = map(int, match.groups()[:3])
            layer = match.group(4) or "default"
            if ident not in defs:
                raise ValueError(f"Unknown brick {ident} in original level")
            if not -1_000_000 < x < 1_000_000 or not -1_000_000 < y < 1_000_000:
                raise ValueError("Invalid brick world coordinate")
            items.append((ident, x, y, layer))

    if not 70 <= len(defs) <= 512 or not 100 <= len(items) <= 40_000:
        raise ValueError("Unexpected brick count; original source may have changed")

    dest = app / "assets" / "level-preview"
    dest.mkdir(parents=True, exist_ok=True)

    # Open Surge's color-key transparency uses exact RGB magenta (#FF00FF).
    # Pillow convert("RGBA") preserves opaque magenta in indexed/opaque PNGs.
    # This was responsible for the giant hot-pink rectangles in v0.2.0.
    def rgba_and_key(path: str):
        with Image.open(root / path) as img:
            rgba = img.convert("RGBA")
        if not (1 <= rgba.width <= 4096 and 1 <= rgba.height <= 4096):
            raise ValueError("Invalid texture dimensions")
        pixels = bytearray(rgba.tobytes())
        keyed = 0
        for i in range(0, len(pixels), 4):
            r, g, b, alpha = pixels[i:i+4]
            if (r, g, b) == (255, 0, 255):
                pixels[i:i+4] = bytes((0, 0, 0, 0))
                keyed += 1
            elif alpha == 0:
                pixels[i:i+3] = bytes((0, 0, 0))
        return rgba.size, pixels, keyed

    def convert(rel: str, out: str):
        size, pixels, keyed = rgba_and_key(rel)
        (dest / out).write_bytes(pixels)
        print(f"ALPHA_KEY {rel}: keyed={keyed}, size={size}")
        return size, pixels

    atlas_size, atlas_pixels = convert("images/sunshine.png", "sunshine.rgba")
    hero_size, hero_pixels = convert("images/players/surge.png", "surge.rgba")

    # Exact Surge starting animation: source_rect 0,32,512,832;
    # frame_size 64x64. Fail rather than silently ship an invisible hero.
    first_frame_alpha = sum(
        hero_pixels[(yy * hero_size[0] + xx) * 4 + 3] != 0
        for yy in range(32, 96) for xx in range(64)
    )
    print(f"SURGE_FIRST_FRAME_VISIBLE_PIXELS={first_frame_alpha}")
    if first_frame_alpha < 20:
        raise ValueError("Original Surge animation frame 0 is unexpectedly blank")

    # Physics collision pixels are derived from the *original* sunshine_mask
    # atlas; magenta and transparent portions must never be collidable.
    mask_size, raw_mask, masked_out = rgba_and_key("images/sunshine_mask.png")
    if mask_size != atlas_size:
        raise ValueError(f"Original Sunshine mask/sprite dimensions differ: {mask_size}, {atlas_size}")
    coverage = bytearray(mask_size[0] * mask_size[1])
    for i in range(len(coverage)):
        coverage[i] = 1 if raw_mask[4*i + 3] >= 128 else 0
    solids = sum(coverage)
    print(f"ORIGINAL_MASK_COVERAGE={solids}/{len(coverage)} keyed={masked_out}")
    if solids < 1000 or solids > len(coverage) * 0.95:
        raise ValueError("Collision-mask opacity is unexpected; inspect original masks")
    (dest / "collision.bin").write_bytes(coverage)
    for i, (x, y, w, h, _, _) in defs.items():
        if x+w > atlas_size[0] or y+h > atlas_size[1]:
            raise ValueError(f"Brick {i} outside atlas {atlas_size}")

    (dest / "header.txt").write_text(
        f"atlas {atlas_size[0]} {atlas_size[1]}\n"
        f"hero {hero_size[0]} {hero_size[1]}\n"
        f"spawn {spawn[1]} {spawn[2]}\n"
        f"instances {len(items)}\n", encoding="ascii"
    )
    (dest / "defs.txt").write_text(
        "".join(f"{i} {x} {y} {w} {h} {typ} {z:.3f}\n"
                for i,(x,y,w,h,typ,z) in sorted(defs.items())), encoding="ascii"
    )
    # Native preview render sorts original depth; the true engine uses more
    # complex z-order rules, which are not reproduced here.
    items.sort(key=lambda x: (defs[x[0]][5], x[2], x[1]))
    # Preserve collision layers: 7 GREEN and 4 YELLOW bricks in demo-1.lev.
    (dest / "bricks.txt").write_text(
        "".join(f"{i} {x} {y} {layer}\n" for i,x,y,layer in items),
        encoding="ascii"
    )
    route_names={"Layer Green":"green","Layer Yellow":"yellow",
                 "Tube In":"tube_in","Tube Out":"tube_out"}
    route_records=[]
    for line in text.splitlines():
        match=re.match(r'^\s*entity\s+"([^"]+)"\s+(-?\d+)\s+(-?\d+)\s+"[^"]+"\s*$',line)
        if match and match.group(1) in route_names:
            route_records.append((route_names[match.group(1)],
                                  int(match.group(2)),int(match.group(3))))
    counts=Counter(name for name,_,_ in route_records)
    assert counts["green"]>=20 and counts["yellow"]>=20, counts
    assert counts["tube_in"]==6 and counts["tube_out"]==6, counts
    (dest/"routes.tsv").write_text(
        "".join(f"{name}\t{x}\t{y}\n" for name,x,y in route_records),
        encoding="ascii")
    print("ORIGINAL_LAYERS",Counter(layer for _,_,_,layer in items))
    print("ORIGINAL_ROUTES",dict(counts))
    summary = {"definitions": len(defs), "instances": len(items),
               "spawn": (int(spawn[1]),int(spawn[2])),
               "theme": theme[1], "atlas": atlas_size, "hero": hero_size,
               "mask_opaque": solids, "magenta_cleared": True,
               "surge_frame_pixels": first_frame_alpha}
    print("Open Surge ORIGINAL data preview:", summary)
    return summary


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("Usage: prepare-level-preview.py <staged-original-game> <app>")
    run(Path(sys.argv[1]), Path(sys.argv[2]))
