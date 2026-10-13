#!/usr/bin/env python3
"""Generate an original 512x512 Starbound Sprint PS5 launcher icon."""
from PIL import Image, ImageDraw, ImageFont
from pathlib import Path
import math

size = 512
img = Image.new("RGBA", (size,size))
p = img.load()
for y in range(size):
    for x in range(size):
        t = y / size
        p[x,y] = (int(8+17*t), int(28-7*t), int(70+35*t), 255)
d = ImageDraw.Draw(img)
for i in range(58):
    x=(i*317+47)%512
    y=(i*151+31)%430
    r=1+(i%3)
    d.ellipse((x-r,y-r,x+r,y+r),fill=(201,241,255,170))
# Circular orbit and spectacular starlight.
d.ellipse((55,81,457,483),outline=(71,238,220,255),width=12)
d.arc((69,89,443,465),-78,42,fill=(255,213,98,255),width=21)
star=[(370,77),(385,113),(423,119),(392,143),(399,181),(370,163),(339,181),(347,143),(317,119),(354,113)]
d.polygon(star,fill=(255,213,98))
# Original original-faced turquoise explorer, with antenna and orange boots.
d.rounded_rectangle((165,254,340,386),radius=42,fill=(26,52,90),outline=(86,246,207),width=12)
d.rounded_rectangle((149,178,354,313),radius=51,fill=(78,239,206))
d.rounded_rectangle((177,211,326,279),radius=28,fill=(13,43,74))
d.ellipse((269,228,304,263),fill=(255,242,168))
d.line((252,178,252,154),fill=(101,247,217),width=10)
d.ellipse((239,135,265,161),fill=(255,216,97))
d.rounded_rectangle((183,376,236,411),radius=13,fill=(255,144,100))
d.rounded_rectangle((270,376,323,411),radius=13,fill=(255,144,100))
dest=Path("app/sce_sys/icon0.png")
dest.parent.mkdir(parents=True,exist_ok=True)
img.convert("RGB").save(dest,format="PNG",optimize=True)
print("Generated",dest)
