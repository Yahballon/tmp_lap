#!/usr/bin/env python3
"""Build native RGBA sprite texture and PS5 launcher icon from concept artpack.

B85 palette-indexed atlas derived from the original Starbound Sprint concept
sheet, not from any NES ROM or third-party commercial game.
"""
from pathlib import Path
from PIL import Image, ImageDraw
import base64
import zlib

HERE = Path(__file__).resolve().parent
APP = Path("app")
SOURCE = HERE / "assets" / "sprite-atlas.b85"
CELL = 16
GRID = 8
SIZE = CELL * GRID
decoded = zlib.decompress(base64.b85decode(SOURCE.read_text().strip()))
if len(decoded) != 1024 + SIZE * SIZE:
    raise ValueError(f"Invalid concept atlas payload size: {len(decoded)}")
palette, indexes = decoded[:1024], decoded[1024:]
pixels = bytearray(SIZE * SIZE * 4)
for i, index in enumerate(indexes):
    pixels[i*4:i*4+4] = palette[index*4:index*4+4]

dest = APP / "assets" / "starbound-sprites.rgba"
dest.parent.mkdir(parents=True, exist_ok=True)
dest.write_bytes(pixels)
assert dest.stat().st_size == SIZE * SIZE * 4
print(f"Created {dest} ({SIZE}x{SIZE} RGBA8; 64 frames)")

# Generate the in-game-branded icon from exactly the same player sprite.
atlas = Image.frombytes("RGBA", (SIZE, SIZE), bytes(pixels))
hero = atlas.crop((0, 0, CELL, CELL))
icon = Image.new("RGB", (512, 512), (6, 22, 46))
d = ImageDraw.Draw(icon)
for i in range(60):
    x=(i*191+29)%512
    y=(i*137+19)%512
    d.ellipse((x,y,x+2,y+2),fill=(95,171,218))
d.ellipse((47,47,465,465),outline=(82,237,215),width=14)
d.ellipse((72,72,440,440),outline=(53,118,181),width=3)
hero = hero.resize((290,290), Image.Resampling.NEAREST)
icon.paste(hero, ((512-290)//2,(512-290)//2), hero)
star = [(402,80),(415,114),(454,121),(422,145),(429,184),(402,162),(373,184),(381,145),(351,121),(388,114)]
d.polygon(star,fill=(255,212,95))
output = APP / "sce_sys" / "icon0.png"
output.parent.mkdir(parents=True,exist_ok=True)
icon.save(output)
print(f"Created {output}")
