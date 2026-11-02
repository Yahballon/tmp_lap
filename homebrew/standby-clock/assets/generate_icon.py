#!/usr/bin/env python3
"""Draw original neon-red clock tile and deploy to the PS5 install tree."""
from pathlib import Path
import sys
from PIL import Image, ImageDraw, ImageFont, ImageFilter
dest=Path(sys.argv[1] if len(sys.argv)>1 else "app/sce_sys/icon0.png")
dest.parent.mkdir(parents=True,exist_ok=True)
im=Image.new("RGB",(512,512),(0,0,0))
glow=Image.new("RGBA",(512,512),(0,0,0,0))
g=ImageDraw.Draw(glow)
g.ellipse((110,108,402,400),outline=(255,0,16,170),width=26)
glow=glow.filter(ImageFilter.GaussianBlur(27))
im=Image.alpha_composite(im.convert("RGBA"),glow)
d=ImageDraw.Draw(im)
red=(255,15,27,255)
d.ellipse((111,108,401,398),outline=red,width=18)
for i in range(60):
 import math
 a=math.pi*i/30-math.pi/2
 start=130 if i%5 else 123
 end=144
 x=256;y=253
 d.line((x+start*math.cos(a),y+start*math.sin(a),x+end*math.cos(a),y+end*math.sin(a)),fill=red,width=3 if i%5 else 6)
d.line((256,253,256,165),fill=red,width=16)
d.line((256,253,332,287),fill=red,width=14)
d.ellipse((246,243,266,263),fill=red)
font_candidates=["/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf","/usr/share/fonts/truetype/liberation2/LiberationSans-Bold.ttf"]
font=next((ImageFont.truetype(p,45) for p in font_candidates if Path(p).exists()),ImageFont.load_default())
d.text((256,443),"STANDBY",font=font,fill=red,anchor="mm")
im.convert("RGB").save(dest,optimize=True)
print("icon",dest,Image.open(dest).size)
