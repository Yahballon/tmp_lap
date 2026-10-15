#!/usr/bin/env python3
"""Create a clean, original 512x512 DreamcastX launcher image using Pillow."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import argparse, math
ap=argparse.ArgumentParser()
ap.add_argument("output",type=Path)
args=ap.parse_args()
size=512
im=Image.new("RGB",(size,size))
p=im.load()
for y in range(size):
    for x in range(size):
        glow=max(0,1-math.hypot(x-256,y-230)/340)
        p[x,y]=(int(8+12*glow),int(18+35*glow),int(36+72*glow))
d=ImageDraw.Draw(im)
for r,al in ((186,1),(155,1),(126,1)):
    c=(32+int(15*al),95+int(18*al),170+int(35*al))
    d.ellipse((256-r,234-r,256+r,234+r),outline=c,width=3)
# Stylised disc orbit + X, no proprietary logos or copyrighted Dreamcast marks.
d.arc((99,76,415,395),start=28,end=306,fill=(94,224,250),width=16)
d.arc((142,118,374,355),start=201,end=356,fill=(37,120,220),width=15)
for offset in (-9,0,9):
    d.line((190+offset,168,316+offset,300),fill=(111,238,255),width=15)
    d.line((316-offset,168,190-offset,300),fill=(111,238,255),width=15)
d.rounded_rectangle((112,398,400,444),radius=16,fill=(12,30,61),outline=(62,115,180),width=2)
try:
    font=ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",21)
except OSError:
    font=ImageFont.load_default()
d.text((256,421),"DREAMCAST X",anchor="mm",fill=(225,242,255),font=font)
args.output.parent.mkdir(parents=True,exist_ok=True)
im.save(args.output,format="PNG",optimize=True)
print("Generated",args.output,"512x512 RGB")
