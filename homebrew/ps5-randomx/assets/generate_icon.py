#!/usr/bin/env python3
"""Generate original 512x512 icon without PlayStation branding."""
from PIL import Image, ImageDraw, ImageFont, ImageFilter
from pathlib import Path
import sys
p=Path(sys.argv[1])
p.parent.mkdir(parents=True,exist_ok=True)
im=Image.new("RGB",(512,512),(3,13,30))
d=ImageDraw.Draw(im)
for y in range(512):
    d.line((0,y,511,y),fill=(2,12+y//35,28+y//21))
glow=Image.new("RGBA",(512,512),(0,0,0,0))
g=ImageDraw.Draw(glow)
g.ellipse((104,82,405,383),outline=(15,218,223,210),width=29)
glow=glow.filter(ImageFilter.GaussianBlur(29))
im=Image.alpha_composite(im.convert("RGBA"),glow)
d=ImageDraw.Draw(im)
d.ellipse((99,78,413,392),fill=(8,34,65),outline=(62,218,233),width=13)
d.ellipse((132,112,380,360),outline=(40,103,174),width=3)
for i in range(13):
    ang=i*6.283185/13
    import math
    x=int(256+124*math.cos(ang));y=int(235+124*math.sin(ang))
    d.ellipse((x-5,y-5,x+5,y+5),fill=(30,171,208))
try: font=ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",110)
except OSError: font=ImageFont.load_default()
d.text((256,226),"RX",font=font,anchor="mm",fill=(244,255,255))
d.rounded_rectangle((135,415,377,467),radius=18,fill=(7,84,108))
try: small=ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",23)
except OSError: small=ImageFont.load_default()
d.text((256,440),"BENCHMARK",font=small,anchor="mm",fill=(205,251,255))
im.convert("RGB").save(p)
