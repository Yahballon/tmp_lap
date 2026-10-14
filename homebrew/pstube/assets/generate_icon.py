#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build PSTUBE's original logo without commercial/trademark artwork."""
import sys
from PIL import Image,ImageDraw,ImageFilter,ImageFont
out=sys.argv[1] if len(sys.argv)>1 else "icon0.png"
im=Image.new("RGBA",(512,512),(2,12,30,255))
glow=Image.new("RGBA",im.size,(0,0,0,0));g=ImageDraw.Draw(glow)
g.rounded_rectangle((44,42,468,470),radius=102,outline=(0,150,255,245),width=14)
im=Image.alpha_composite(im,glow.filter(ImageFilter.GaussianBlur(16)))
d=ImageDraw.Draw(im)
d.rounded_rectangle((42,43,470,469),radius=105,fill=(2,25,61),outline=(0,142,255),width=13)
d.ellipse((96,105,420,420),fill=(7,54,117))
d.rounded_rectangle((151,167,362,226),radius=28,fill=(36,214,255))
d.polygon([(309,193),(361,218),(317,276),(233,276),(233,380),(177,380),(177,211)],fill=(34,170,255))
d.polygon([(166,190),(268,267),(166,350)],fill=(255,64,97))
im.convert("RGB").save(out)
