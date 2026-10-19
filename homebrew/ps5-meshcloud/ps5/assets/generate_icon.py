#!/usr/bin/env python3
"""Original MeshCloud launcher icon. No console/vendor marks."""
from PIL import Image, ImageDraw, ImageFilter
from pathlib import Path
import sys

out=Path(sys.argv[1] if len(sys.argv)>1 else "app/sce_sys/icon0.png")
out.parent.mkdir(parents=True,exist_ok=True)
im=Image.new("RGB",(512,512))
draw=ImageDraw.Draw(im)
for y in range(512):
    draw.line((0,y,512,y),fill=(3,10+y//62,27+y//18))
glow=Image.new("RGBA",(512,512),(0,0,0,0))
g=ImageDraw.Draw(glow)
g.ellipse((80,80,432,432),outline=(38,153,255,195),width=28)
im=Image.alpha_composite(im.convert("RGBA"),glow.filter(ImageFilter.GaussianBlur(33)))
d=ImageDraw.Draw(im)
d.rounded_rectangle((96,96,416,416),radius=65,fill=(9,34,73,255),outline=(52,155,255,255),width=8)
nodes=[(171,184),(340,163),(218,333),(363,332)]
for a,b in [(0,1),(0,2),(0,3),(1,3),(2,3)]:
    d.line((*nodes[a],*nodes[b]),fill=(52,156,246,255),width=18)
for x,y in nodes:
    d.ellipse((x-26,y-26,x+26,y+26),fill=(19,95,188),outline=(180,231,255),width=8)
d.ellipse((220,222,293,294),fill=(44,216,170),outline=(205,254,239),width=7)
im.convert("RGB").save(out,optimize=True)
