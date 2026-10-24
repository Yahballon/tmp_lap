#!/usr/bin/env python3
from PIL import Image,ImageDraw,ImageFont
import sys
im=Image.new('RGB',(512,512))
px=im.load()
for y in range(512):
 for x in range(512):
  px[x,y]=(4+int(y*.01),13+int(y*.025),30+int(y*.055))
d=ImageDraw.Draw(im)
d.rounded_rectangle((25,25,486,486),radius=52,outline=(46,230,251),width=9)
for y in range(120,400,38):d.rounded_rectangle((252,y,260,y+23),radius=3,fill=(80,100,135))
d.rounded_rectangle((65,210,93,330),radius=12,fill=(54,239,252))
d.rounded_rectangle((419,185,447,305),radius=12,fill=(255,66,135))
d.ellipse((220,256,255,291),fill=(245,255,255))
try:f=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',53)
except OSError:f=ImageFont.load_default()
d.text((84,68),'P2P PONG',font=f,fill=(237,250,255))
im.save(sys.argv[1])
