#!/usr/bin/env python3
"""Draw an original CPU diagnostic icon; uses no brand trademarks."""
from PIL import Image,ImageDraw,ImageFilter
from pathlib import Path
import sys
p=Path(sys.argv[1] if len(sys.argv)>1 else 'app/sce_sys/icon0.png')
p.parent.mkdir(parents=True,exist_ok=True)
im=Image.new('RGB',(512,512),(2,12,33))
d=ImageDraw.Draw(im)
for y in range(512):
    d.line((0,y,512,y),fill=(3,12+int(y/38),39+int(y/28)),width=1)
glow=Image.new('RGBA',(512,512));g=ImageDraw.Draw(glow)
g.rounded_rectangle((96,99,417,415),radius=63,outline=(0,139,255,165),width=32)
glow=glow.filter(ImageFilter.GaussianBlur(28))
im=Image.alpha_composite(im.convert('RGBA'),glow)
d=ImageDraw.Draw(im)
d.rounded_rectangle((99,99,413,413),radius=55,fill=(10,48,112,255),outline=(90,195,255,255),width=11)
for v in range(6):
    c=153+v*41
    d.rounded_rectangle((c-8,69,c+8,108),radius=5,fill=(57,157,255))
    d.rounded_rectangle((c-8,405,c+8,444),radius=5,fill=(57,157,255))
    d.rounded_rectangle((70,c-8,109,c+8),radius=5,fill=(57,157,255))
    d.rounded_rectangle((405,c-8,444,c+8),radius=5,fill=(57,157,255))
d.rounded_rectangle((153,153,359,359),radius=27,fill=(4,24,59),outline=(90,192,255),width=7)
for k in range(4):
    x=179+k*44
    d.line((x,193,x,314),fill=(27,83,147),width=2)
    d.line((190,189+k*44,322,189+k*44),fill=(27,83,147),width=2)
d.line([(192,290),(228,250),(260,267),(321,201)],fill=(55,241,178),width=15,joint='curve')
d.ellipse((299,178,342,221),fill=(68,241,181))
im.convert('RGB').save(p)
