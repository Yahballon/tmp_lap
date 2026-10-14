#!/usr/bin/env python3
"""Generate deterministic high-resolution, copyright-original PS5 game assets.

Run AFTER generate-original-media.py. 2x source atlas dimensions preserve
Gold872's 16-pixel cell IDs; upgrade-renderer.py doubles source rectangles.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter
import math, random

ROOT=Path(__file__).resolve().parents[1]/'res'/'sprites'
random.seed(872)
atlases=[
 ('characters/PlayerSpriteSheet.png',25,16),
 ('characters/EnemySpriteSheet.png',35,15),
 ('blocks/BlockTileSheet.png',48,22),
]
for filename,cols,rows in atlases:
    p=ROOT/filename
    im=Image.open(p).convert('RGBA')
    assert im.size==(cols*16,rows*16), (p,im.size)
    out=Image.new('RGBA',(cols*32,rows*32),(0,0,0,0))
    for y in range(rows):
        for x in range(cols):
            cell=im.crop((x*16,y*16,x*16+16,y*16+16))
            # Crisp source-pixel doubling followed by distinct material highlights.
            cell=cell.resize((32,32),Image.Resampling.NEAREST)
            d=ImageDraw.Draw(cell)
            if 'blocks/' in filename:
                i=y*cols+x
                if i not in (101,149,152,144,145,146,147,192,193,194,195):
                    rng=random.Random(i)
                    for j in range(5):
                        px=rng.randrange(4,29); py=rng.randrange(8,29)
                        d.line((px,py,px+rng.choice((1,2,3)),py),fill=(120,151,181,190))
                    if i//48 in (0,1,2,3,4,5):
                        d.line((2,2,29,2),fill=(110,237,209,255),width=2)
                        for j in range(4):
                            px=rng.randrange(3,29)
                            d.line((px,3,px,rng.randrange(4,11)),fill=(43,177,109,255),width=2)
            elif 'Player' in filename:
                d.point((24,8),fill=(245,255,255,255))
                d.line((13,19,17,19),fill=(72,247,238,255),width=2)
            else:
                d.point((12,11),fill=(255,255,255,255))
                d.point((23,11),fill=(255,255,255,255))
            out.alpha_composite(cell,(x*32,y*32))
    out.save(p,optimize=True)

W,H=800,480
sky=Image.new('RGBA',(W,H))
pix=sky.load()
for y in range(H):
    t=y/(H-1)
    c=(int(12+102*t),int(45+143*t),int(110+129*t),255)
    for x in range(W): pix[x,y]=c
d=ImageDraw.Draw(sky,'RGBA')
# Atmosphere and large ringed moon; avoid any third-party visual assets.
d.ellipse((465,50,740,325),fill=(112,201,248,165),outline=(198,244,255,220),width=3)
for r in range(0,120,12):
    d.arc((474+r//3,60+r//3,731-r//3,317-r//3),194,292,fill=(202,245,255,42),width=2)
d.ellipse((638,93,687,141),fill=(148,225,254,150))
d.arc((392,116,875,357),195,345,fill=(225,248,255,120),width=4)
rng=random.Random(1616)
for i in range(130):
    x=rng.randrange(W);y=rng.randrange(280)
    alpha=rng.randrange(55,155)
    d.point((x,y),fill=(255,255,255,alpha))
# Distant clouds, cliffs and layered mountains as parallax-like silhouettes.
for layer,(baseline,color,height) in enumerate((
    (340,(73,153,218,160),72),(394,(52,127,197,200),113),(468,(35,103,165,240),103))):
    points=[(0,H),(0,baseline)]
    for x in range(0,W+12,12):
        ridge=math.sin(x/73+layer*3)*height*.31+math.sin(x/31+layer)*height*.15
        ridge+=rng.randrange(-8,9)
        points.append((x,int(baseline-ridge)))
    points.extend([(W,H)])
    d.polygon(points,fill=color)
for i in range(15):
    x=rng.randrange(W); y=rng.randrange(180,385)
    w=rng.randrange(55,165)
    d.ellipse((x,y,x+w,y+rng.randrange(10,28)),fill=(220,248,255,rng.randrange(40,100)))
p=ROOT/'backgrounds'/'alien_sky.png'
p.parent.mkdir(parents=True,exist_ok=True)
sky.save(p,optimize=True)
print('HD atlases and 800x480 layered alien sky created')
