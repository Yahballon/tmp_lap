#!/usr/bin/env python3
"""Reproducible fallback icon for CI if the checked-in AI PNG is unavailable."""
from PIL import Image, ImageDraw, ImageFont, ImageFilter
from pathlib import Path
import sys, math
destination=Path(sys.argv[1]) if len(sys.argv)>1 else Path("icon0.png")
im=Image.new("RGB",(512,512),(12,26,43))
d=ImageDraw.Draw(im)
for y in range(512):
 v=int(12*math.sin(y*0.025))
 d.line((0,y,511,y),fill=(max(0,12+v),max(0,24+v),max(0,43+v)))
d.rounded_rectangle((15,15,497,497),radius=66,outline=(70,186,243),width=13)
d.rounded_rectangle((48,320,464,366),radius=13,fill=(112,144,171),outline=(204,230,246),width=6)
for i in range(7):
 x=64+i*58
 d.rounded_rectangle((x,113,x+46,321),radius=8,fill=(36+i*8,59+i*8,83+i*7),outline=(143,170,196),width=4)
 d.line((x+11,145,x+35,145),fill=(193,224,244),width=4)
d.arc((169,99,350,319),175,367,fill=(77,195,255),width=26)
d.rounded_rectangle((166,235,200,304),radius=16,fill=(149,182,204))
d.rounded_rectangle((312,235,346,304),radius=16,fill=(149,182,204))
d.rounded_rectangle((212,170,303,319),radius=6,fill=(16,39,59),outline=(90,202,249),width=5)
for i in range(7):
 x=230+i*10;h=[17,29,44,62,44,29,17][i]
 d.line((x,260-h//2,x,260+h//2),fill=(78,215,255),width=4)
try:
 font=ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",33)
 small=ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",23)
except OSError:font=small=ImageFont.load_default()
d.text((256,406),"AUDIOBOOK",anchor="mm",font=font,fill=(224,244,255))
d.text((256,451),"by YAHBALLON",anchor="mm",font=small,fill=(94,201,248))
destination.parent.mkdir(parents=True,exist_ok=True)
im.save(destination)
