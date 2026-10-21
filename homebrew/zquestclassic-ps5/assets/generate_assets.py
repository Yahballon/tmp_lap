#!/usr/bin/env python3
"""Render original procedural tile art for the native PS5 launcher.

This is not AI-generated artwork and is not an upstream Zelda asset.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import sys

out = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("app")
target = out / "sce_sys" / "icon0.png"
target.parent.mkdir(parents=True, exist_ok=True)
im = Image.new("RGB", (512, 512), "#08171b")
d = ImageDraw.Draw(im)
for y in range(0, 512, 24):
    d.line((0, y, 512, y), fill="#0e2629", width=1)
for x in range(0, 512, 24):
    d.line((x, 0, x, 512), fill="#0e2629", width=1)
d.rounded_rectangle((27, 27, 485, 485), radius=57, outline="#65efab", width=7)
for points in [
    [(68,123),(191,123),(191,165),(110,165),(110,247),(156,247),(156,286),(68,286)],
    [(330,127),(443,127),(443,283),(406,283),(406,164),(367,164),(367,209),(330,209)],
    [(111,345),(194,345),(194,385),(151,385),(151,429),(111,429)],
    [(324,341),(438,341),(438,429),(397,429),(397,383),(324,383)],
]:
    d.polygon(points, fill="#317967")
sword = [(250,92),(348,185),(318,225),(289,198),(289,337),(314,373),
         (256,440),(198,373),(223,337),(223,198),(194,225),(164,185)]
d.polygon(sword, fill="#e5bf76")
d.line(sword + [sword[0]], fill="#fff0bb", width=6, joint="curve")
d.polygon([(256,163),(281,195),(269,327),(256,345),(243,327),(231,195)], fill="#0a2928")
d.ellipse((239,106,273,140), fill="#fff8c4")
for y in (255,278):
    d.line((168 if y == 255 else 186, y, 344 if y == 255 else 326, y),
           fill="#72e6b0", width=8)
font_paths = [
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    "/usr/share/fonts/truetype/liberation2/LiberationSans-Bold.ttf"
]
font_path = next((p for p in font_paths if Path(p).is_file()), None)
font = ImageFont.truetype(font_path, 22) if font_path else ImageFont.load_default()
d.text((256, 460), "ZQUEST CLASSIC", anchor="mm", font=font, fill="#d5f8e3")
im.save(target, optimize=True)
with Image.open(target) as check:
    assert check.size == (512, 512) and check.format == "PNG"
print("Wrote original procedural icon:", target)
