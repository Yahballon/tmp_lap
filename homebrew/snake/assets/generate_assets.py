#!/usr/bin/env python3
"""Generate original 16:9 neon-jungle Snake menu art and launcher icon.

All art is generated procedurally at build time. Pillow is a CI build dependency.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageFilter
import math
import random
import sys

out=Path(sys.argv[1]) if len(sys.argv)>1 else Path("app")
(out/"sce_sys").mkdir(parents=True,exist_ok=True)
(out/"assets").mkdir(parents=True,exist_ok=True)
rng=random.Random(12088)
W,H=1920,1080
scene=Image.new("RGB",(W,H));draw=ImageDraw.Draw(scene)
for y in range(H):
    p=y/H
    col=(int(1+3*p),int(15+9*p),int(18+1*p))
    draw.line((0,y,W,y),fill=col)
# Bluish mist between a distant jungle and the foreground.
mist=Image.new("RGBA",(W,H),(0,0,0,0));md=ImageDraw.Draw(mist)
for i in range(150):
    x=rng.randrange(W);y=rng.randrange(H);r=rng.randrange(20,140)
    c=rng.choice([(16,62,86,25),(26,65,76,23),(43,102,38,18)])
    md.ellipse((x-r,y-r,x+r,y+r),fill=c)
scene=Image.alpha_composite(scene.convert("RGBA"),mist.filter(ImageFilter.GaussianBlur(25)))
foliage=Image.new("RGBA",(W,H),(0,0,0,0));fd=ImageDraw.Draw(foliage)
# Silhouette leaf canopies, hanging vines, and undergrowth. Transparent enough to
# keep all interactive screen areas readable.
for side in (-1,1):
    for i in range(95):
        x=rng.randrange(-120,370) if side<0 else rng.randrange(1590,2020)
        y=rng.randrange(-140,1130)
        sz=rng.randrange(24,125)
        color=rng.choice([(4,57,33,170),(14,77,37,165),(17,95,36,135),(2,33,37,190)])
        fd.polygon([(x,y),(x+side*sz*.4,y-sz*.65),(x+side*sz*1.5,y-sz*.1),
                    (x+side*sz*.8,y+sz*.3)],fill=color)
for i in range(100):
    x=rng.randrange(W);y=rng.choice([rng.randrange(-20,170),rng.randrange(930,1200)])
    sz=rng.randrange(30,170);shade=rng.choice([(8,51,26,140),(25,94,38,120),(1,31,23,180)])
    fd.polygon([(x,y),(x-sz*.5,y+sz*.5),(x+sz*.3,y+sz),(x+sz*.8,y+sz*.4)],fill=shade)
scene=Image.alpha_composite(scene,foliage.filter(ImageFilter.GaussianBlur(7)))
scene=Image.alpha_composite(scene,foliage)
# Atmospheric cobra graphic on the upper-right; painted on a small canvas and
# scaled with nearest-neighbor filtering for the recognizable pixel-snake artwork.
cw,ch=108,86
cobra=Image.new("RGBA",(cw,ch),(0,0,0,0));c=ImageDraw.Draw(cobra)
dark=(15,61,30,205);rim=(45,115,44,204);hi=(64,138,51,145)
c.polygon([(11,10),(31,2),(76,4),(97,18),(105,43),(94,49),(79,31),
           (69,24),(53,21),(29,28),(15,45),(9,57),(9,77),(23,77),
           (19,85),(2,85),(0,57)],fill=dark)
c.line([(10,16),(31,5),(75,8),(95,20),(103,42)],fill=rim,width=6,joint="curve")
c.line([(12,49),(21,28),(48,22),(73,26),(89,41),(85,65),
        (71,76),(65,85)],fill=rim,width=7,joint="curve")
c.polygon([(38,29),(57,27),(86,49),(71,55),(64,73),(36,72),(25,59)],fill=(3,19,20,222))
c.rectangle((68,13,76,19),fill=(86,175,48,220))
c.rectangle((70,14,74,19),fill=(1,12,9,240))
for i in range(210):
    x=rng.randrange(cw);y=rng.randrange(ch)
    if cobra.getpixel((x,y))[3]>0 and rng.randrange(4)==0:
        c.rectangle((x,y,x+1,y+1),fill=hi if i%2==0 else (13,72,33,150))
cobra=cobra.resize((735,584),Image.Resampling.NEAREST)
scene.alpha_composite(cobra,(1410,-18))
# Tight cinematic vignette.
vignette=Image.new("RGBA",(W,H));vg=ImageDraw.Draw(vignette)
for i in range(38):
    a=int(2+i*.60)
    vg.rounded_rectangle((i*4,i*4,W-i*4,H-i*4),radius=170,
                         outline=(0,0,0,a),width=5)
scene=Image.alpha_composite(scene,vignette.filter(ImageFilter.GaussianBlur(7)))
# Strong 3D logo title, based on the composition (not a copied logo image).
paths=["/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
       "/usr/share/fonts/truetype/liberation2/LiberationSans-Bold.ttf"]
fontpath=next((p for p in paths if Path(p).exists()),None)
def font(size):
    return ImageFont.truetype(fontpath,size) if fontpath else ImageFont.load_default()
logo=Image.new("RGBA",(650,280),(0,0,0,0));ld=ImageDraw.Draw(logo)
for off in range(19,0,-2):
    ld.text((29+off,41+off),"SNAKE",font=font(126),fill=(0,25+off*3,13,255),
            stroke_width=7,stroke_fill=(0,30,12,255))
ld.text((29,42),"SNAKE",font=font(126),fill=(115,255,30,255),
        stroke_width=7,stroke_fill=(8,100,20,255))
ld.text((30,37),"SNAKE",font=font(126),fill=(198,255,93,255),
        stroke_width=2,stroke_fill=(73,225,31,255))
logo=logo.rotate(7,Image.Resampling.BICUBIC,expand=False)
glow=logo.filter(ImageFilter.GaussianBlur(20))
scene.alpha_composite(glow,(3,77))
scene.alpha_composite(logo,(3,77))
ld=ImageDraw.Draw(scene)
ld.text((185,311),"BY YAHBALLON",font=font(26),fill=(214,255,241,255))
ld.text((408,315),"v0.1.0",font=font(17),fill=(98,198,126,255))
# A few luminous motes, located outside gameplay frame.
spots=Image.new("RGBA",(W,H));sd=ImageDraw.Draw(spots)
for i in range(65):
    x=rng.randrange(W);y=rng.randrange(H);r=rng.randrange(2,8)
    sd.ellipse((x-r,y-r,x+r,y+r),fill=rng.choice([(100,252,77,35),(94,147,255,25)]))
scene=Image.alpha_composite(scene,spots.filter(ImageFilter.GaussianBlur(15)))
scene.convert("RGB").save(out/"assets/jungle.jpg",quality=89,optimize=True,subsampling=0)
# Installer icon: native original pixel-snake scene.
s=512
im=Image.new("RGB",(s,s),(1,17,18));d=ImageDraw.Draw(im)
for y in range(0,s,24):d.line((0,y,s,y),fill=(7,49,35),width=1)
for x in range(0,s,24):d.line((x,0,x,s),fill=(7,49,35),width=1)
d.rounded_rectangle((23,23,489,489),radius=24,outline=(56,255,71),width=10)
blocks=[(120,320),(160,320),(200,320),(240,320),(280,320),(320,320),
        (320,280),(320,240),(320,200)]
for i,(x,y) in enumerate(blocks):
    d.rounded_rectangle((x,y,x+37,y+37),radius=5,fill=(100,255,20) if i==8 else (61,219,18),
                        outline=(155,255,72),width=3)
d.rectangle((345,204,353,211),fill=(1,65,15))
d.ellipse((132,202,179,251),fill=(255,51,67))
d.ellipse((140,207,149,216),fill=(255,209,202))
d.line((155,196,161,184),fill=(84,224,29),width=6)
d.text((256,74),"SNAKE",font=font(67),anchor="mm",fill=(150,255,48),
       stroke_width=3,stroke_fill=(2,82,18))
d.text((256,445),"BY YAHBALLON",font=font(21),anchor="mm",fill=(213,255,226))
im.save(out/"sce_sys/icon0.png",optimize=True)
print("Generated native neon jungle backdrop (1920x1080) and PS5 icon (512x512)")
