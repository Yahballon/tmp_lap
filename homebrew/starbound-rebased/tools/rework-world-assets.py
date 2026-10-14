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

# New original side-scrolling level layouts. 8 worlds, four distinct stages
# each, Gold872's CSV layers and level property parser remain intact.
# This does NOT copy any Nintendo level map or proprietary art.
for world in range(1,9):
    for stage in range(1,5):
        rng=random.Random(19500+world*311+stage*17)
        width=144+world*10+(stage%2)*16
        height=15
        layers={name:[[-1]*width for _ in range(height)] for name in (
          "Above_Foreground","Background","Collectibles",
          "Enemies","Foreground","Underground")}
        fg=layers["Foreground"]
        material=[2,50,98,146,194,242,290,338][world-1]
        gap_starts=[]
        # Guarantee a safe entry runway and exit. Intermediate gaps get
        # a coherent, varied rhythm and never swallow both sides of a jump.
        for x in range(width):
            gap=(x>22 and x<width-22 and (x+(world*7+stage*11))%41 in range(0,2+stage%3))
            if not gap:
                fg[13][x]=material
                fg[14][x]=material+1
            else: gap_starts.append(x)
        for x in range(21,width-22,17+stage):
            y=7+((x//17+stage+world)%4)
            n=2+(stage+world+x)%4
            for j in range(n):
                fg[y][x+j]=material+2+(j%2)
            for j in range(n):
                if (j%2==0 and y>0):
                    layers["Collectibles"][y-1][x+j]=144+(j%4)
        for x in range(32,width-24,29):
            if fg[13][x] !=-1:
                layers["Enemies"][12][x]=(world+stage)%4
        for x in range(12,width-13,24):
            if fg[13][x] != -1 and (x+1 <width and fg[13][x+1] !=-1):
                # Rising bridge / stepped structures rather than disconnected
                # random single tiles. Keep traversal feasible by bounding height.
                h=2+(world+stage+x//24)%3
                for y in range(13-h,13):
                    fg[y][x]=material+2
                    fg[y][x+1]=material+3
        # Original destination gate (not the source game's level geometry).
        gate=width-13
        for y in range(7,13):fg[y][gate]=101
        fg[7][gate+1]=152
        for y in range(9,13):fg[y][width-5]=material+2
        # Sparse background stones; they are NOT collidable.
        for x in range(9,width-12,15):
            layers["Background"][11][x]=material+4
        path=R/"data"/f"World{world}-{stage}"
        path.mkdir(parents=True,exist_ok=True)
        for name,rows in layers.items():
            (path/f"World{world}-{stage}_{name}.csv").write_text(
                "\n".join(",".join(str(z) for z in row) for row in rows)+"\n")
        nw,ns=(world,stage+1) if stage<4 else ((world+1,1) if world<8 else (0,0))
        props=f"""PLAYER_START = (2, 12)
CAMERA_START = (0, 0)
CAMERA_MAX = {width-25}
LEVEL_TYPE = OVERWORLD
NEXT_LEVEL = ({nw}, {ns})
BACKGROUND_COLOR = BLUE
FLOATING_TEXT=\\
WARP_PIPE=\\
MOVING_PLATFORM=\\
PLATFORM_LEVEL=\\
FIRE_BAR=\\
VINE=\\
"""
        (path/f"World{world}-{stage}.levelproperties").write_text(props)
print("Fixed original 2x block atlas, transparent UI/logo and 32 authored levels")
