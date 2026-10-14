#!/usr/bin/env python3
"""Make the original Gold872-compatible (NOT Nintendo) PS5 assets actually usable.

Run after generate-original-media.py and upgrade-media.py. Preserve 2x
atlas geometry (48x22 block cells, 25x16 player, 35x15 enemy).
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import math, random
R=Path(__file__).resolve().parents[1]/"res"
S=R/"sprites"
def font(size):
    for f in ("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
              "/usr/share/fonts/truetype/liberation2/LiberationSans-Bold.ttf"):
        if Path(f).exists(): return ImageFont.truetype(f,size)
    return ImageFont.load_default()

# The old logo was 460x96, then compressed into a 352x192 rectangle.
# Generate at its *display aspect ratio* with all text within a safe margin.
im=Image.new("RGBA",(704,384),(0,0,0,0))
d=ImageDraw.Draw(im)
d.rounded_rectangle((20,42,684,342),radius=42,fill=(8,22,48,238),
                    outline=(69,225,231,255),width=10)
d.rounded_rectangle((39,61,665,323),radius=29,outline=(51,119,176,255),width=3)
d.text((352,145),"STARBOUND",font=font(79),fill="#e8ffff",anchor="mm",
       stroke_width=1,stroke_fill="#12385b")
d.text((352,233),"REBASED",font=font(80),fill="#6af2e2",anchor="mm")
im.save(S/"icons/logo.png",optimize=True)
# All options panels use transparent, properly-sized PNGs; v0.1.10 painted
# an 800x480 opaque image into a 448x96 rectangle, hiding the entire menu.
for name in ("optionsbackground.png","optionsinfobackground.png"):
    p=S/"icons"/name
    im=Image.new("RGBA",(448,128),(0,0,0,0))
    d=ImageDraw.Draw(im)
    d.rounded_rectangle((3,3,444,125),radius=18,fill=(8,20,47,238),
                        outline=(48,216,225,255),width=4)
    im.save(p,optimize=True)

# Distinct, readable, original sci-fi tile materials. Never replace each
# atlas index with the same metal square. Original Gold872 atlas shape / IDs
# and animation addresses stay unchanged.
atlas=S/"blocks/BlockTileSheet.png"
im=Image.open(atlas).convert("RGBA")
assert im.size == (1536,704)
for tileid in range(48*22):
    col,row=tileid%48,tileid//48
    x0,y0=col*32,row*32
    # Preserve special gameplay objects (coins, pipes, animated pickup frames)
    # at their exact addresses, but recolor generic terrain by world palette.
    if tileid in (101,149,152,144,145,146,147,192,193,194,195):
        continue
    p=random.Random(tileid*677+41)
    terrain=row%11
    if col>=32 and row<10: # non-standard atlas groups
        continue
    # Map broad atlas groups to biome materials. Avoid an all-identical sheet.
    if terrain in (0,1,2): top=(80,222,183); face=(42,84,123); edge=(15,43,69)
    elif terrain in (3,4): top=(248,181,109); face=(120,81,101); edge=(66,43,72)
    elif terrain in (5,6): top=(158,161,246); face=(70,75,146); edge=(36,47,94)
    elif terrain in (7,8): top=(111,215,243); face=(36,98,132); edge=(18,59,99)
    else: top=(188,239,123); face=(66,117,97); edge=(25,63,69)
    q=Image.new("RGBA",(32,32),face+(255,));a=ImageDraw.Draw(q)
    a.rectangle((0,0,31,3),fill=top+(255,))
    a.line((0,4,0,31),fill=edge+(255,),width=2)
    a.line((2,31,31,31),fill=edge+(255,),width=2)
    for z in range(3):
        xx=p.randrange(4,26);yy=p.randrange(8,28)
        a.line((xx,yy,xx+p.randint(2,6),yy+p.randint(-1,1)),
               fill=(max(0,face[0]-16),max(0,face[1]-11),max(0,face[2]-9),255),width=2)
    if col in (1,5,9,13):
        a.ellipse((11,13,19,21),outline=top+(255,),width=2)
    if col in (2,6,10):
        a.polygon([(7,26),(15,13),(24,26)],outline=(168,224,239,255))
    im.paste(q,(x0,y0))
im.save(atlas,optimize=True)

# Campaign maps are installed separately, after all reference/raster work.
print("Prepared original block atlas and transparent UI assets")
