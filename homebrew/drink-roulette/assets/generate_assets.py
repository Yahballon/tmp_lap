#!/usr/bin/env python3
"""Original generated neon wheel tile and party room for PS5 homebrew."""
from PIL import Image, ImageDraw, ImageFilter
from pathlib import Path
import math
import random
import sys
out=Path(sys.argv[1]) if len(sys.argv)>1 else Path('app')
(out/'sce_sys').mkdir(parents=True,exist_ok=True)
(out/'assets').mkdir(parents=True,exist_ok=True)

# App tile: neon bottle spinner, 10 bright wheel slices and title.
s=768
im=Image.new('RGBA',(s,s),(7,6,21,255))
d=ImageDraw.Draw(im,'RGBA')
for y in range(s):
 d.line((0,y,s,y),fill=(10+int(8*y/s),6+int(5*y/s),32+int(15*y/s),255))
colors=['#ffbb28','#ad2dff','#17baff','#3bfc8d','#ff8629','#ff3b68','#c233ff','#ffce20','#198cff','#ff3f79']
cx,cy,r=s//2,350,273
lay=Image.new('RGBA',(s,s));ld=ImageDraw.Draw(lay)
for i,color in enumerate(colors):
 start=-108+i*36
 ld.pieslice((cx-r,cy-r,cx+r,cy+r),start=start,end=start+35,fill=color,outline='#ffffff',width=2)
ld.ellipse((cx-133,cy-133,cx+133,cy+133),fill='#17101a',outline='#ffd994',width=7)
ld.ellipse((cx-r-7,cy-r-7,cx+r+7,cy+r+7),outline='#ffbf49',width=12)
glow=lay.filter(ImageFilter.GaussianBlur(24))
im=Image.alpha_composite(im,glow)
im=Image.alpha_composite(im,lay)
d=ImageDraw.Draw(im,'RGBA')
d.polygon([(cx-205,cy-12),(cx-174,cy-28),(cx+90,cy-28),(cx+120,cy-12),(cx+210,cy-9),(cx+210,cy+9),(cx+120,cy+12),(cx+90,cy+28),(cx-174,cy+28)],fill='#009b48')
d.line((cx-162,cy-21,cx+70,cy-21),fill='#aaffc4',width=8)
d.rounded_rectangle((58,655,710,741),radius=23,fill=(18,13,32,240),outline='#f72be8',width=4)
from PIL import ImageFont
paths=['/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf','/usr/share/fonts/truetype/liberation2/LiberationSans-Bold.ttf']
font=next((p for p in paths if Path(p).exists()),None)
if font:
 f=ImageFont.truetype(font,60)
 d.text((s//2,696),'DRINK ROULETTE',font=f,anchor='mm',fill='#ffecff',stroke_width=2,stroke_fill='#ac29b7')
im.convert('RGB').resize((512,512),Image.Resampling.LANCZOS).save(out/'sce_sys/icon0.png')

# A generic party-scene background used for CI; the exact reference photo is optional.
w,h=1920,1080
scene=Image.new('RGB',(w,h),(8,8,21))
d=ImageDraw.Draw(scene)
for y in range(h):
 f=y/h
 d.line((0,y,w,y),fill=(int(3+52*f),int(7+12*f),int(24+5*f)))
# wall wood panels, curtains, skyline
for i in range(15):
 x=i*150
 d.line((x,0,x,655),fill=(19,16,30),width=4)
d.rounded_rectangle((1120,190,1870,475),radius=58,fill=(21,16,39),outline=(46,34,58),width=8)
d.rounded_rectangle((1240,350,1790,520),radius=30,fill=(26,31,48))
d.rounded_rectangle((1273,370,1498,462),radius=27,fill=(63,35,57))
d.rounded_rectangle((1516,376,1730,466),radius=29,fill=(22,63,52))
d.polygon([(0,535),(1920,490),(1920,1080),(0,1080)],fill=(54,29,23))
# bowls and paper cups in the distant room
for i in range(11):
 x=900+i*94;y=520+i%3*54
 d.polygon([(x,y),(x+48,y),(x+38,y+71),(x+12,y+71)],fill=(123+((i%3)*24),23,32))
 d.line((x-2,y,x+51,y),fill=(255,133,89),width=6)
# strands of fairy lights and scattered glowy bokeh
rng=random.Random(31)
glow_layer=Image.new('RGBA',(w,h));g=ImageDraw.Draw(glow_layer)
for i in range(31):
 x=i*66;y=28+int(27*math.sin(i*.7))
 g.ellipse((x-13,y-13,x+13,y+13),fill=(255,186,75,200))
for i in range(65):
 x=rng.randrange(w);y=rng.randrange(20,830);r=rng.randint(7,33)
 g.ellipse((x-r,y-r,x+r,y+r),fill=rng.choice([(241,48,140,40),(255,162,38,75),(23,120,255,35)]))
scene=Image.alpha_composite(scene.convert('RGBA'),glow_layer.filter(ImageFilter.GaussianBlur(23)))
scene=Image.alpha_composite(scene,glow_layer)
scene.convert('RGB').save(out/'assets/generated_scene.jpg',quality=88,optimize=True)
print('Created original icon and fallback party backdrop')
