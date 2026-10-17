#!/usr/bin/env python3
"""Original high-resolution Bible Reader icon for the installed PS5 menu."""
from PIL import Image, ImageDraw, ImageFilter
import math, pathlib, sys

def create(output):
    size = 512
    im=Image.new("RGB",(size,size))
    pixels=im.load()
    for y in range(size):
        for x in range(size):
            radial=max(0,1-math.hypot(x-256,y-252)/470)
            pixels[x,y]=(int(2+7*radial),int(9+26*radial),int(26+52*radial))
    gold=(250,198,97)
    glow=Image.new("RGBA",(size,size),(0,0,0,0))
    g=ImageDraw.Draw(glow)
    # Large, recognisable opened book silhouette, designed for TV icon grids.
    g.polygon([(74,313),(103,188),(242,215),(258,246),(273,215),(408,188),(439,313),(265,347),(255,358),(246,347)],fill=(236,179,73,240))
    glow=glow.filter(ImageFilter.GaussianBlur(22))
    im=Image.alpha_composite(im.convert("RGBA"),glow)
    d=ImageDraw.Draw(im)
    d.rounded_rectangle((42,43,470,469),radius=42,outline=(139,108,58,190),width=3)
    d.polygon([(71,320),(107,183),(246,211),(256,241),(268,211),(405,183),(442,320),(262,349),(251,349)],fill=(9,27,55),outline=gold,width=9)
    d.line([(256,241),(256,347)],fill=(255,223,139),width=9)
    for n in range(3):
        pad=15*n
        d.arc((103+pad,195+pad,250,330+pad),197,318,fill=(238,194,94,210-n*45),width=3)
        d.arc((261,195+pad,406-pad,330+pad),225,343,fill=(238,194,94,210-n*45),width=3)
    d.ellipse((239,224,273,258),fill=(255,243,177))
    d.line([(256,137),(256,226)],fill=(255,222,151),width=12)
    d.line([(220,167),(292,167)],fill=(255,222,151),width=12)
    output=pathlib.Path(output);output.parent.mkdir(parents=True,exist_ok=True)
    im.convert("RGB").save(output,format="PNG",optimize=True)
if __name__=="__main__":
    create(sys.argv[1])
