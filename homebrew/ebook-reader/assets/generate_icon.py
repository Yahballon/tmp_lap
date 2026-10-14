#!/usr/bin/env python3
from PIL import Image, ImageDraw
import math,sys
p=sys.argv[1] if len(sys.argv)>1 else 'icon0.png'
im=Image.new('RGB',(512,512),(38,19,8));d=ImageDraw.Draw(im)
for y in range(512):
 v=int(12*math.sin(y*.05)+6*math.sin(y*.19))
 d.line((0,y,512,y),fill=(min(255,111+v),max(0,57+v),max(0,21+v//2)))
d.rounded_rectangle((20,20,492,492),radius=70,outline=(238,169,69),width=16)
for j in range(6,0,-1):
 o=3*j
 d.polygon([(65-o,150+o),(210-o,151+o),(256,184+o),(307+o,151+o),(447+o,150+o),(445+o,346+o),(309,330+o),(256,357+o),(209,330+o),(66-o,346+o)],fill=(92+j*15,50+j*12,27+j*9),outline=(236,173,77))
d.polygon([(75,159),(211,151),(256,177),(256,361),(210,332),(75,341)],fill=(249,226,172))
d.polygon([(256,177),(305,151),(437,159),(437,341),(309,332),(256,361)],fill=(255,239,195))
d.line([(256,177),(256,361)],fill=(141,81,32),width=8)
for k in range(8):
 y=190+k*18
 d.line((105,y,213,y-5),fill=(170,126,77),width=2)
 d.line((305,y-5,409,y),fill=(170,126,77),width=2)
im.save(p)
