#!/usr/bin/env python3
"""Create original 512x512 tracker icon. No platform marks or trademark logos."""
from PIL import Image, ImageDraw

im=Image.new("RGB",(512,512),(4,14,39))
d=ImageDraw.Draw(im)
for i in range(16):
    d.rounded_rectangle((34+i,34+i,478-i,478-i),radius=104-i,
                        outline=(8, max(50,170-i*6), max(90,240-i*7)),width=2)
d.rounded_rectangle((50,50,462,462),radius=94,fill=(8,32,79),outline=(14,193,255),width=10)
points=[(90,302),(148,257),(208,278),(273,191),(332,212),(387,122),(426,151)]
d.line(points,fill=(58,215,255),width=20,joint="curve")
for x,y in points:d.ellipse((x-10,y-10,x+10,y+10),fill=(231,250,255))
for x,h in [(128,327),(204,302),(280,267),(356,238)]:
    d.rounded_rectangle((x-9,h,x+9,388),radius=5,fill=(37,137,239))
im.save("app/sce_sys/icon0.png")
