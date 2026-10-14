#!/usr/bin/env python3
"""Native-resolution cosmetic replacement for the pinned Gold872 atlas layout.

This is intentionally NOT a guessed 32px tile grid. The original reference
PNG controls canvas dimensions and every pixel's alpha, including 2-high
sprites, connected hill/cloud outlines, particle frames and pipe caps.
Only RGB colors are changed, using the supplied pink/cyan HD art palette.
No upstream pixels, palettes, names, or RGB sprite drawings are copied.
"""
from pathlib import Path
from PIL import Image
import numpy as np
import hashlib
import sys

repo = Path(__file__).resolve().parents[1]
ref = Path(sys.argv[1] if len(sys.argv) > 1 else "build/gold872-reference") / "res"
target = repo / "res" / "sprites"
AI_PINK = np.array((250, 45, 162), dtype=np.float32)
AI_PINK_HI = np.array((255, 153, 216), dtype=np.float32)
AI_TEAL = np.array((15, 227, 213), dtype=np.float32)
AI_TEAL_HI = np.array((161, 255, 247), dtype=np.float32)
AI_NAVY = np.array((9, 15, 47), dtype=np.float32)
AI_GREEN = np.array((20, 191, 152), dtype=np.float32)
AI_GREEN_HI = np.array((111, 245, 188), dtype=np.float32)
AI_WHITE = np.array((240, 253, 255), dtype=np.float32)
AI_CLOUD_SHADOW = np.array((116, 209, 244), dtype=np.float32)

def ids(path):
    result = set()
    for line in path.read_text().splitlines():
        for v in line.split(","):
            v = int(v)
            if v >= 0:
                result.add(v)
    return result

background_ids = ids(ref / "data/World1-1/World1-1_Background.csv")
background_ids |= ids(ref / "data/MenuBackground/MenuBackground_Background.csv")

atlases = [
    ("characters/PlayerSpriteSheet.png", (403, 266), 25, 16, 16, 16, 1, 9),
    ("characters/EnemySpriteSheet.png", (560, 240), 35, 15, 16, 16, 1, 1),
    ("blocks/BlockTileSheet.png", (817, 375), 48, 22, 17, 17, 1, 1),
]

def recolor_cell(cell, atlas, sprite_id, is_background):
    arr = np.asarray(cell.convert("RGBA"), dtype=np.uint8)
    rgb = arr[..., :3].astype(np.float32)
    alpha = arr[..., 3]
    # Work on isolated original grid cells, never draw outside their existing
    # alpha mask. In particular, clouds and hills keep their internal cut-outs.
    luma = rgb[..., 0] * .299 + rgb[..., 1] * .587 + rgb[..., 2] * .114
    strength = np.clip((luma - 36.0) / 210.0, 0.0, 1.0)
    dark = (luma < 52.0) & (alpha > 0)
    white = (np.min(rgb, axis=2) > 177) & (np.max(rgb, axis=2) - np.min(rgb, axis=2) < 65)
    green = (rgb[..., 1] > rgb[..., 0] * 1.24) & (rgb[..., 1] > 85)
    blue = (rgb[..., 2] > rgb[..., 0] * 1.15) & (rgb[..., 2] > 90)
    w = strength[..., None]
    if atlas == "player":
        # The original transparent silhouette and source rectangles remain
        # unchanged, so big-player (16x32) spans, jumps and FIREBALL frame IDs
        # cannot accidentally become two full-height little astronauts.
        low, hi = AI_PINK, AI_PINK_HI
        if sprite_id in (246, 247, 248, 249) or (sprite_id // 25 == 9 and sprite_id % 25 >= 20):
            low, hi = AI_TEAL, AI_TEAL_HI
        colored = low[None, None, :] * (1.0 - w) + hi[None, None, :] * w
        colored = np.where(blue[..., None] | white[..., None],
                           AI_TEAL[None,None,:] * (1.0 - w) + AI_TEAL_HI[None,None,:] * w,
                           colored)
    elif atlas == "enemy":
        low, hi = AI_PINK, AI_PINK_HI
        colored = low[None,None,:] * (1.0-w) + hi[None,None,:] * w
        colored = np.where(green[..., None] | blue[..., None],
                           AI_TEAL[None,None,:] * (1.0-w)+AI_TEAL_HI[None,None,:]*w,
                           colored)
    else:
        # Background cells are clouds/hills/bushes, NOT interchangeable
        # solid blocks. Original irregular silhouettes decide visibility.
        if is_background:
            colored = AI_GREEN[None,None,:]*(1.0-w) + AI_GREEN_HI[None,None,:]*w
            cloud = AI_CLOUD_SHADOW[None,None,:]*(1.0-w)+AI_WHITE[None,None,:]*w
            colored = np.where(white[...,None], cloud, colored)
        else:
            colored = AI_PINK[None,None,:]*(1.0-w)+AI_PINK_HI[None,None,:]*w
            special = AI_TEAL[None,None,:]*(1.0-w)+AI_TEAL_HI[None,None,:]*w
            colored = np.where(blue[...,None] & (luma > 70)[...,None], special, colored)
            colored = np.where(white[...,None], AI_WHITE, colored)
    # Crisp original transparent outlines; retain original alpha byte-by-byte.
    colored = np.where(dark[..., None], AI_NAVY[None,None,:], colored)
    out = arr.copy()
    out[..., :3] = np.clip(np.rint(colored), 0, 255).astype(np.uint8)
    out[..., 3] = alpha
    return Image.fromarray(out, "RGBA")

for name, size, columns, rows, pitchx, pitchy, ox, oy in atlases:
    original = Image.open(ref / "sprites" / name).convert("RGBA")
    assert original.size == size, (name, original.size, size)
    atlas = "player" if "Player" in name else "enemy" if "Enemy" in name else "block"
    output = Image.new("RGBA", original.size, (0, 0, 0, 0))
    # Copy alpha for the entire canvas (including special sprite rectangles
    # crossing row boundaries) and then recolor only samples inside sourceRect.
    base = np.asarray(original, dtype=np.uint8).copy()
    base[..., :3] = 0
    output = Image.fromarray(base, "RGBA")
    for row in range(rows):
        for col in range(columns):
            index = row * columns + col
            x, y = ox + col * pitchx, oy + row * pitchy
            x2, y2 = min(size[0], x+16), min(size[1], y+16)
            if x2 <= x or y2 <= y:
                continue
            cell = original.crop((x,y,x2,y2))
            if not cell.getchannel("A").getbbox():
                continue
            change = recolor_cell(cell, atlas, index, index in background_ids)
            output.paste(change, (x,y), change.getchannel("A"))
    # PIL paste with a mask would square an alpha edge. Always overwrite alpha
    # with the *original*, preserving every animation and collision silhouette.
    out = np.asarray(output, dtype=np.uint8).copy()
    out[...,3] = np.asarray(original.getchannel("A"))
    final = Image.fromarray(out, "RGBA")
    assert final.size == original.size
    assert final.getchannel("A").tobytes() == original.getchannel("A").tobytes()
    dest = target / name
    dest.parent.mkdir(parents=True, exist_ok=True)
    final.save(dest, optimize=True)
    assert Image.open(dest).size == size
    assert hashlib.sha256(final.tobytes()).digest() != hashlib.sha256(original.tobytes()).digest()
    print("NATIVE ATLAS OK:", name, size, "grid", columns, "x", rows,
          "origin", (ox,oy), "pitch", (pitchx,pitchy))
print("All 3 native Gold872 atlases reskinned; all reference alpha pixels preserved.")
