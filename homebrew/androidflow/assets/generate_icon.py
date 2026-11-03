#!/usr/bin/env python3
"""Deterministic original AndroidFlow tile illustration (not AI-generated)."""
from pathlib import Path
from PIL import Image, ImageDraw
import sys

def generate(path):
    image = Image.new("RGB", (512, 512))
    pix = image.load()
    for y in range(512):
        for x in range(512):
            glow = max(0, 1 - ((x - 255)**2 + (y - 238)**2)**0.5/400)
            pix[x, y] = (int(8 + 24*glow), int(16 + 43*glow), int(32 + 53*glow))
    d = ImageDraw.Draw(image, "RGBA")
    for dx in (-124, 124):
        x = 256 + dx
        d.rounded_rectangle((x-62, 156, x+62, 352), radius=22,
                            fill=(38, 91, 110, 230), outline=(91, 210, 205, 190), width=5)
        d.rounded_rectangle((x-43, 177, x+43, 274), radius=21, fill=(9, 25, 44, 255))
        d.ellipse((x-28, 198, x+28, 254), fill=(91, 230, 209, 225))
    d.rounded_rectangle((148, 115, 364, 392), radius=35,
                        fill=(26, 60, 88, 255), outline=(96, 243, 216, 255), width=8)
    d.rounded_rectangle((174, 145, 338, 310), radius=33, fill=(9, 20, 38, 255))
    d.ellipse((195, 166, 317, 288), fill=(65, 217, 190, 255))
    d.rounded_rectangle((190, 328, 322, 349), radius=10, fill=(97, 211, 204, 230))
    d.rounded_rectangle((219, 364, 292, 373), radius=4, fill=(177, 213, 223, 190))
    d.ellipse((113, 390, 400, 432), fill=(33, 177, 170, 40))
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    image.save(path, "PNG", optimize=True)

if __name__ == "__main__":
    generate(sys.argv[1])
