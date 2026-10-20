#!/usr/bin/env python3
"""Original transparent glass-bottle sprite with reflections for PS5 native roulette."""
from PIL import Image,ImageDraw,ImageFilter
from pathlib import Path
import math,sys
out=Path(sys.argv[1]);out.parent.mkdir(parents=True,exist_ok=True)
w,h=1000,300
image=Image.new('RGBA',(w,h),(0,0,0,0))
# Contact shadow is softly blurred and moves with bottle.
sh=Image.new('RGBA',(w,h),(0,0,0,0));d=ImageDraw.Draw(sh)
d.ellipse((90,113,890,240),fill=(0,0,0,110))
image.alpha_composite(sh.filter(ImageFilter.GaussianBlur(20)))
mask=Image.new('L',(w,h),0);md=ImageDraw.Draw(mask)
outline=[(85,83),(102,67),(136,55),(603,55),(655,75),(695,116),(866,116),(876,122),(876,169),(866,178),(696,178),(655,214),(603,235),(139,235),(102,222),(85,201)]
md.polygon(outline,fill=255)
md.rounded_rectangle((68,85,119,204),radius=19,fill=255)
base=Image.new('RGBA',(w,h),(0,0,0,0));pix=base.load()
for y in range(h):
 for x in range(50,900):
  t=(y-145)/92
  edge=max(0,1-abs(t))
  lens=0.23+0.5*edge + 0.12*math.sin(x/37)
  r=int(2+15*lens);g=int(33+119*lens);b=int(17+48*lens)
  pix[x,y]=(r,g,b,255)
base.putalpha(mask)
image.alpha_composite(base)
d=ImageDraw.Draw(image,'RGBA')
# Edges, translucent heel, specular strips and refraction.
d.line(outline+[outline[0]],fill=(106,255,144,185),width=5,joint='curve')
d.arc((68,84,121,204),90,270,fill=(159,253,173,220),width=6)
d.line((132,72,599,72),fill=(203,255,205,188),width=15)
d.line((135,85,593,85),fill=(255,255,235,121),width=4)
d.line((148,115,580,112),fill=(26,224,118,80),width=13)
d.line((140,211,589,211),fill=(6,15,11,183),width=15)
d.line((158,221,580,221),fill=(126,252,149,99),width=4)
d.arc((589,64,719,226),260,90,fill=(193,251,181,155),width=5)
d.line((690,121,857,122),fill=(218,250,221,192),width=8)
d.line((706,173,858,170),fill=(2,22,12,170),width=8)
d.line((685,145,865,143),fill=(157,255,194,70),width=5)
# Embossed ivory-gold art label with curved translucency.
d.rounded_rectangle((394,76,488,209),radius=16,fill=(239,197,111,138),outline=(252,229,161,212),width=4)
d.line((412,80,411,205),fill=(255,250,181,121),width=9)
d.line((471,80,472,205),fill=(119,80,24,117),width=7)
d.arc((420,118,469,168),0,360,fill=(249,216,117,180),width=6)
d.ellipse((439,137,453,151),fill=(252,247,219,190))
# Metallic gold cap at the narrow end.
d.rounded_rectangle((859,108,900,184),radius=9,fill=(167,99,28,246),outline=(255,224,139,255),width=5)
for k in range(6):
 x=863+k*6
 d.line((x,118,x,174),fill=(240,192,82,110),width=3)
d.line((881,109,890,183),fill=(255,255,211,154),width=5)
# Sparkles and tiny condensation droplets.
for x,y,r in [(223,108,5),(246,187,4),(325,163,3),(545,104,5),(576,193,4),(720,137,3)]:
 d.ellipse((x-r,y-r,x+r,y+r),fill=(229,255,222,125))
 d.ellipse((x-r/2,y-r/2,x+r/4,y+r/4),fill=(255,255,255,151))
# Store in a square transparent atlas: the renderer rotates UVs in-place.
atlas=Image.new('RGBA',(1024,1024),(0,0,0,0))
resized=image.resize((780,234),Image.Resampling.LANCZOS)
atlas.alpha_composite(resized,(160,395))
atlas.save(out)
print('Built square bottle atlas',out)
