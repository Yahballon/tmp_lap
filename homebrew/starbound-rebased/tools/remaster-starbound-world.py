#!/usr/bin/env python3
"""Starbound Rebased: original sci-fi world art, native Gold872 atlas addressing.

Only the texture assets change. Every PNG keeps its upstream native canvas,
individual sourceRect geometry, 17px block pitch and 1px gutter. No level,
collision, animation-ID, or controller changes are made here.

Unlike a palette swap, this script draws wholly NEW sci-fi tiles, a new space
sky, and maps the user's AI characters across the full source sprite groups.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter
from collections import Counter
import random, math, sys, hashlib

ROOT=Path(__file__).resolve().parents[1]
ORIGINAL=Path(sys.argv[1]) if len(sys.argv)>1 else Path('build/gold872-reference')
SPR=ROOT/'res/sprites'
AI=ROOT/'hd-assets/characters'
SCALE=Image.Resampling.NEAREST
PINK=(242,44,163,255); PINKHI=(255,146,213,255)
DEEP=(16,16,52,255); BLUE=(24,79,151,255); CYAN=(0,234,215,255)
WHITE=(230,254,255,255); SHADOW=(7,24,61,255); PURPLE=(88,42,153,255)


def atlas_cell(im, x,y,w=16,h=16):
    assert x>=0 and y>=0 and x+w<=im.width and y+h<=im.height,(x,y,w,h,im.size)
    return im.crop((x,y,x+w,y+h))


def stamp(dest,x,y,image,w=None,h=None):
    w=w or image.width;h=h or image.height
    assert 0<=x and 0<=y and x+w<=dest.width and y+h<=dest.height,(x,y,w,h)
    dest.paste((0,0,0,0),(x,y,x+w,y+h))
    dest.paste(image,(x,y))


def ai_sample(src,col,row,full_h=32):
    # Existing user AI sheets have 32x32 frames, or two high cells for larger foes.
    x=(col%35)*32+(2 if src.width==806 else 0)
    y=row*32+(18 if src.width==806 else 0)
    return src.crop((x,y,x+32,y+full_h))


def suit(src,col,row,large=False):
    art=ai_sample(src,col,row)
    if not art.getchannel('A').getbbox():
        art=ai_sample(src,col%9,0)
    if not large: return art.resize((16,16),SCALE)
    # Stretch physical body + legs into ONE continuous 16x32 sprite.
    top=art.crop((0,0,32,22)).resize((16,21),SCALE)
    feet=art.crop((0,22,32,32)).resize((16,11),SCALE)
    out=Image.new('RGBA',(16,32));out.paste(top,(0,0));out.paste(feet,(0,21))
    return out


def glowing_ball(hit=False):
    im=Image.new('RGBA',(16,16));d=ImageDraw.Draw(im)
    if hit:
        d.polygon([(8,0),(10,5),(15,3),(12,8),(15,12),(9,12),(8,16),(5,12),(1,14),(4,8),(1,4),(6,5)],fill=CYAN)
        d.point((7,7),fill=WHITE)
    else:
        d.ellipse((2,2,12,12),fill=SHADOW,outline=CYAN,width=2)
        d.rectangle((5,4,9,8),fill=WHITE)
    return im


def draw_players():
    source=Image.open(AI/'PlayerSpriteSheet.png').convert('RGBA')
    assert source.size==(806,532),source.size
    dest=Image.new('RGBA',(403,266))
    # Gold872 frame IDs: four small+large pairs of 3 rows each, plus specials.
    # Group starts at 0/3/6/9/12/15. The following row contains a SINGLE
    # 16x32 powered-up frame spanning that and the next atlas row.
    for start in (0,3,6,9,12,15):
        for col in range(25):
            y=9+start*16
            if y+16>dest.height: continue
            group=(start//3)%8
            stamp(dest,1+col*16,y,suit(source,col,group))
    for start in (1,4,7,10,13):
        for col in range(25):
            x=1+col*16;y=9+start*16
            if y+32>dest.height:continue
            group=((start-1)//3)%8
            # Unlike before, large frames NEVER contain two small avatars.
            stamp(dest,x,y,suit(source,col,group,True),16,32)
    # Animation IDs 246 and 247 are 8x8 projectiles, NOT astronauts.
    for col,hit in ((21,False),(22,True)):
        x=1+col*16;y=9+9*16
        stamp(dest,x,y,glowing_ball(hit).resize((8,8),SCALE),16,16)
    # Power-up effects near 245 and other special projectile slots.
    for row,col in ((9,23),(9,24)):
        x=1+col*16;y=9+row*16
        stamp(dest,x,y,glowing_ball(col==24))
    assert dest.size==(403,266)
    dest.save(SPR/'characters/PlayerSpriteSheet.png',optimize=True)
    return dest


def draw_enemies():
    source=Image.open(AI/'EnemySpriteSheet.png').convert('RGBA')
    assert source.size==(1120,480)
    dest=Image.new('RGBA',(562,242))
    # Each variant row is sampled from original AI slime/crawler/drone/plant
    # artwork. No recolored Koopa, Goomba, or original Mario silhouette remains.
    for r in range(15):
        for c in range(35):
            if r<=2:sr=0
            elif r<=4:sr=3
            elif r<=6:sr=5
            elif r<=8:sr=7
            elif r<=10:sr=9
            else:sr=0 if r%2==0 else 5
            tile=ai_sample(source,(c+3*r)%35,sr).resize((16,16),SCALE)
            if not tile.getchannel('A').getbbox():
                tile=ai_sample(source,c%10,0).resize((16,16),SCALE)
            stamp(dest,1+c*16,1+r*16,tile)
    # The supported 16x32 armored enemies (Koopa IDs 38/39) form ONE
    # vertical character; don't independently draw the bottom frame beneath.
    for c in (3,4):
        long=ai_sample(source,c,3,64).resize((16,32),SCALE)
        stamp(dest,1+c*16,1+16,long,16,32)
    # 70/71 slimes live at row 2, col 0/1 (not at the top row).
    for c in (0,1):
        stamp(dest,1+c*16,1+2*16,ai_sample(source,c,0).resize((16,16),SCALE))
    assert dest.size==(562,242)
    dest.save(SPR/'characters/EnemySpriteSheet.png',optimize=True)
    return dest


def parse_used(p):
    ids=set()
    if p.exists():
        for ln in p.read_text().splitlines():
            for s in ln.split(','):
                try:k=int(s.strip())
                except ValueError:continue
                if k>=0:ids.add(k)
    return ids


def tech_tile(idx):
    tile=Image.new('RGBA',(16,16));d=ImageDraw.Draw(tile)
    rr=random.Random(idx*1921+13)
    is_ground=idx in (384,385,386,387) or idx in range(432,448)
    supply=idx in (192,193,194,195,53,69,581,608)
    coin=idx in range(144,149) or idx in range(96,101)
    conduit=idx in range(149,154) or idx in range(240,253) or idx in range(292,297)
    platforms=idx in (197,198,199,289,290,291,101,152)
    if is_ground:
        # Solid crystalline strata, coherent even when repeated in rows.
        d.rectangle((0,0,15,15),fill=(43,23,82,255))
        d.line((0,1,15,1),fill=PINKHI,width=2)
        d.polygon([(0,5),(4,3),(7,6),(11,3),(16,6),(16,16),(0,16)],fill=(104,29,114,255))
        d.line((0,8,4,11,10,8,15,11),fill=(196,62,152,255),width=1)
        d.line((2,14,5,12,9,14),fill=(20,158,162,255),width=1)
        d.point((12,12),fill=PINKHI)
    elif supply:
        # Unlockable energy crate, NO question-mark/NES silhouette.
        d.rounded_rectangle((0,0,15,15),radius=2,fill=(28,36,96,255),outline=CYAN,width=1)
        d.rectangle((2,2,13,13),fill=(81,43,126,255),outline=PINK,width=1)
        d.polygon([(8,3),(12,8),(8,12),(4,8)],fill=CYAN)
        d.line((5,8,11,8),fill=WHITE,width=1)
        for xy in ((1,1),(14,1),(1,14),(14,14)):d.point(xy,fill=PINKHI)
    elif coin:
        # Floating power crystal / collectible; transparent around it.
        d.polygon([(8,1),(13,6),(10,14),(5,14),(2,6)],fill=CYAN,outline=SHADOW)
        d.line((8,2,8,12),fill=WHITE)
        d.point((6,5),fill=WHITE)
    elif conduit:
        # Slim energy reactor tower sections; visually not a Mario pipe.
        d.rounded_rectangle((2,0,13,15),radius=2,fill=(21,44,95,255),outline=(17,182,196,255),width=1)
        d.rectangle((5,0,10,15),fill=(55,78,157,255))
        d.line((7,1,7,14),fill=CYAN,width=2)
        for yy in (2,12):d.line((2,yy,13,yy),fill=PINKHI,width=1)
        d.rectangle((0,0,15,2),fill=(115,64,179,255))
    elif platforms:
        d.rectangle((0,0,15,15),fill=(38,32,88,255))
        d.line((0,0,15,0),fill=PINKHI,width=2)
        d.line((0,15,15,15),fill=(17,183,197,255),width=2)
        d.rectangle((2,3,13,12),outline=(137,69,175,255),width=1)
        d.line((4,11,11,4),fill=CYAN)
        d.point((5,5),fill=WHITE)
    else:
        # Varied industrial plates, chassis and ribbed support blocks.
        mode=(idx//48)%5
        base=[(35,37,88,255),(33,64,108,255),(65,41,109,255),(61,29,91,255),(26,67,79,255)][mode]
        d.rectangle((0,0,15,15),fill=base)
        d.line((0,0,15,0),fill=(100,127,195,255),width=1)
        d.line((0,0,0,15),fill=(27,220,207,255),width=1)
        d.line((0,15,15,15),fill=SHADOW,width=2)
        d.line((15,0,15,15),fill=SHADOW,width=2)
        if idx%3==0:
            d.rectangle((3,3,12,12),outline=PINK,width=1)
            d.polygon([(4,11),(7,5),(10,11)],fill=CYAN)
        elif idx%3==1:
            d.rectangle((5,2,10,13),fill=(22,26,64,255))
            d.line((6,4,9,4),fill=CYAN)
            d.line((6,10,9,10),fill=PINKHI)
        else:
            d.line((2,11,6,7,10,7,14,3),fill=CYAN,width=1)
            d.ellipse((2,3,5,6),fill=PINKHI)
    return tile


def draw_tiles():
    (SPR/'blocks').mkdir(parents=True,exist_ok=True)
    original=Image.open(ORIGINAL/'res/sprites/blocks/BlockTileSheet.png').convert('RGBA')
    assert original.size==(817,375)
    bg=parse_used(ORIGINAL/'res/data/World1-1/World1-1_Background.csv')
    bg|=parse_used(ORIGINAL/'res/data/MenuBackground/MenuBackground_Background.csv')
    kept=set()
    for layer in ('Foreground','Above_Foreground','Underground','Collectibles'):
        kept |= parse_used(ORIGINAL/f'res/data/World1-1/World1-1_{layer}.csv')
    # Replacing all background tiles with the bespoke sky avoids recreating
    # the original cloud/hill/bush scene silhouette.
    background_only=bg-kept
    dest=Image.new('RGBA',(817,375))
    made=0
    for idx in range(48*22):
        x=1+(idx%48)*17;y=1+(idx//48)*17
        reference=original.crop((x,y,x+16,y+16))
        rgb=reference.convert('RGB')
        if idx in background_only:continue
        # Upstream atlas empty cells use a flat transparent blue color-key.
        colors=rgb.getdata()
        if not any(c!=(147,187,236) and c!=(0,0,0) for c in colors):
            if idx not in kept:continue
        tile=tech_tile(idx)
        # Completely new art within the 16x16 source cell, ORIGINAL index.
        dest.paste(tile,(x,y))
        made+=1
    dest.save(SPR/'blocks/BlockTileSheet.png',optimize=True)
    assert dest.size==(817,375)
    for idx in (384,192,197,289,149,150):
        r=dest.crop((1+(idx%48)*17,1+(idx//48)*17,17+(idx%48)*17,17+(idx//48)*17))
        assert r.getchannel('A').getbbox(),('required visible tile blank',idx)
    return made,len(background_only)


def draw_sky():
    W,H=800,480
    img=Image.new('RGBA',(W,H))
    pix=img.load()
    # Deeper blue alien dusk, not classic pale-blue NES backdrop.
    for y in range(H):
        t=y/(H-1)
        col=(int(5+21*t),int(13+46*t),int(39+73*t),255)
        for x in range(W):pix[x,y]=col
    d=ImageDraw.Draw(img)
    rng=random.Random(87021)
    # Star points, distant galaxies and asteroid belts.
    for n in range(135):
        x=rng.randrange(800);y=rng.randrange(30,330)
        bright=rng.choice((110,150,200,255))
        d.point((x,y),fill=(bright,bright,255,255))
        if n%23==0:
            d.line((x-2,y,x+2,y),fill=(99,219,251,255))
            d.line((x,y-2,x,y+2),fill=(99,219,251,255))
    # Ringed planet to the far right. Side silhouettes not game tiles.
    d.ellipse((535,84,662,211),fill=(56,31,122,255),outline=(114,73,197,255),width=5)
    for k in range(4):d.arc((540+k*4,91+k*3,657-k*4,204-k*3),205,347,fill=(133,77,190,255),width=2)
    d.ellipse((591,102,622,133),fill=(173,86,195,255))
    d.arc((510,131,686,178),194,357,fill=CYAN,width=3)
    d.arc((510,133,686,180),12,165,fill=(60,143,215,255),width=2)
    # Neon mesa mountains, 100% custom angular geometry, no face-like holes.
    for ybase,maxh,color in ((407,95,(28,39,89,255)),(452,70,(31,54,112,255)),(483,46,(38,42,92,255))):
        poly=[(0,H),(0,ybase)]
        for x in range(0,817,32):
            h=int((math.sin(x/91+ybase)+1)*maxh*.25 + (math.sin(x/47)*.5+.5)*maxh*.35)
            poly.append((x,ybase-h))
        poly.append((W,H));d.polygon(poly,fill=color)
    # Irregular monoliths and sci-fi settlement on the far horizon.
    for x,width,hh in ((75,17,54),(145,9,27),(278,16,40),(327,25,66),(727,18,50)):
        yy=421-hh
        d.rectangle((x,yy,x+width,460),fill=(16,25,67,255))
        d.line((x,yy,x+width,yy),fill=(31,143,176,255),width=2)
        for row in range(yy+8,442,9):
            d.line((x+width//2-2,row,x+width//2+2,row),fill=CYAN)
    for x,y in ((120,343),(245,365),(396,371),(695,360)):
        d.line((x-2,y,x+2,y),fill=CYAN)
    dest=SPR/'backgrounds/alien_sky.png';dest.parent.mkdir(parents=True,exist_ok=True)
    img.save(dest,optimize=True)
    return img


def main():
    (SPR/'characters').mkdir(parents=True,exist_ok=True)
    p=draw_players();e=draw_enemies();count,hidden=draw_tiles();sky=draw_sky()
    assert (p.size,e.size,sky.size)==((403,266),(562,242),(800,480))
    print('STARBOUND V0.1.18 ORIGINAL SCI-FI REMASTER: player and foes=',p.size,e.size)
    print('new game block tiles:',count,'hidden old Mario background IDs:',hidden,'space backdrop:',sky.size)

if __name__=='__main__':main()