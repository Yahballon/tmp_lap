#!/usr/bin/env python3
"""Original PS5 launcher art: neon cyan/purple speed gauge and globe.
Generated offline with Pillow; no copyrighted console logos."""
from PIL import Image, ImageDraw, ImageFont, ImageFilter
from pathlib import Path
import math
SIZE=512
im=Image.new("RGB",(SIZE,SIZE),(2,9,29))
d=ImageDraw.Draw(im)
for y in range(SIZE):
    d.line((0,y,SIZE,y),fill=(2,9+int(y/100),28+int(y/36)))
d.rounded_rectangle((20,20,492,492),radius=82,outline=(11,121,236),width=7)
d.rounded_rectangle((31,31,481,481),radius=75,outline=(29,209,255),width=2)
cx,cy=256,222
for r in [188,183,177]:
    d.arc((cx-r,cy-r,cx+r,cy+r),194,342,fill=(10,194,255),width=7 if r==188 else 2)
    d.arc((cx-r,cy-r,cx+r,cy+r),343,347,fill=(205,62,255),width=7)
    d.arc((cx-r,cy-r,cx+r,cy+r),0,78,fill=(205,62,255),width=6)
for i in range(16):
    theta=math.radians(194+i*(232/15))
    x1=cx+157*math.cos(theta);y1=cy+157*math.sin(theta)
    x2=cx+171*math.cos(theta);y2=cy+171*math.sin(theta)
    d.line((x1,y1,x2,y2),fill=(160,226,255),width=4 if i%3==0 else 2)
d.ellipse((132,100,380,348),fill=(4,32,97),outline=(33,168,255),width=4)
for angle in (-60,-30,0,30,60):
    rx=123*abs(math.cos(math.radians(angle)))
    d.ellipse((cx-rx,100,cx+rx,348),outline=(12,95,172),width=2)
for y in (135,166,222,278,313):
    q=math.sqrt(max(0,123**2-(y-cy)**2))
    d.line((cx-q,y,cx+q,y),fill=(21,111,192),width=2)
angle=math.radians(321)
tip=(cx+162*math.cos(angle),cy+162*math.sin(angle))
d.line((cx,cy,tip[0],tip[1]),fill=(14,205,255),width=17)
d.line((cx,cy,tip[0],tip[1]),fill=(239,247,255),width=5)
d.ellipse((239,205,273,239),fill=(5,18,57),outline=(44,213,255),width=5)
try:
    font=ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",66)
    small=ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",37)
except OSError:
    font=small=ImageFont.load_default()
d.rounded_rectangle((58,361,454,462),radius=24,fill=(1,12,42),outline=(16,116,240),width=3)
d.text((256,371),"WWW",font=font,anchor="mt",fill=(232,247,255),stroke_width=1,stroke_fill=(21,118,224))
d.text((256,434),"SPEEDTEST",font=small,anchor="mm",fill=(98,218,255))
out=Path("app/sce_sys/icon0.png")
out.parent.mkdir(parents=True,exist_ok=True)
im.save(out,optimize=True)
print(f"Generated {out}: {im.size}")
