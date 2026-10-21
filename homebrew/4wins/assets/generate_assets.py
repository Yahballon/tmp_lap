#!/usr/bin/env python3
"""Draw original neon-lounge scene and package PS5 raster assets."""
from PIL import Image,ImageDraw,ImageFont,ImageFilter
from pathlib import Path
import math,random,sys
out=Path(sys.argv[1]); (out/'assets').mkdir(parents=True,exist_ok=True);(out/'sce_sys').mkdir(parents=True,exist_ok=True)
W,H=1920,1080
random.seed(4)
base=Image.new('RGB',(W,H))
d=ImageDraw.Draw(base)
for y in range(H):
    f=y/H
    d.line((0,y,W,y),fill=(int(3+17*f),int(9+7*f),int(29+31*f)))
d.rectangle((0,750,W,H),fill=(6,10,34))
for x,col in [(10,(244,22,73)),(880,(8,80,245)),(1250,(1,174,255)),(1890,(231,83,24))]:
    glow=Image.new('RGBA',(W,H),(0,0,0,0));g=ImageDraw.Draw(glow)
    g.line((x,0,x,720),fill=(*col,150),width=12)
    base=Image.alpha_composite(base.convert('RGBA'),glow.filter(ImageFilter.GaussianBlur(40))).convert('RGB')
    d=ImageDraw.Draw(base);d.line((x,0,x,720),fill=col,width=7)
d=ImageDraw.Draw(base)
d.rounded_rectangle((1150,490,1920,825),radius=70,fill=(18,23,45),outline=(25,42,78),width=10)
d.rounded_rectangle((30,510,450,775),radius=60,fill=(17,19,39),outline=(34,45,80),width=10)
for y in range(775,1080,28):d.line((0,y,1920,y),fill=(int(18+(y-775)*.04),15,39),width=2)
for x in range(0,1920,45):d.line((x,790,x-120,1080),fill=(33,18,47),width=1)
fontpaths=['/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf','/usr/share/fonts/truetype/liberation2/LiberationSans-Bold.ttf']
fp=next((x for x in fontpaths if Path(x).exists()),None)
def font(n):return ImageFont.truetype(fp,n) if fp else ImageFont.load_default()
d.text((70,42),'4 WINS',font=font(115),fill=(235,242,255),stroke_width=8,stroke_fill=(12,36,147))
d.text((83,175),'by YAHBALLON',font=font(25),fill=(133,197,255))
for i,label in enumerate(['Start Game','Game Mode','Settings','Exit']):
    y=283+i*100
    d.rounded_rectangle((58,y,468,y+76),radius=18,fill=(5,24,67),outline=(38,112,222),width=3)
    d.text((133,y+22),label,font=font(29),fill=(245,245,255))
board=(565,208,1370,839)
d.rounded_rectangle(board,radius=26,fill=(11,51,174),outline=(58,165,255),width=12)
for r in range(6):
    for c in range(7):
        x=625+c*112;y=277+r*99
        d.ellipse((x-43,y-43,x+43,y+43),fill=(2,9,35),outline=(40,133,245),width=8)
for c,r,p in [(0,0,1),(1,0,2),(2,0,1),(3,0,1),(4,0,2),(5,0,2),(3,1,2),(3,2,1),(2,1,2)]:
    x=625+c*112;y=772-r*99
    d.ellipse((x-40,y-40,x+40,y+40),fill=(241,32,69) if p==1 else (255,203,29),outline=(255,231,178) if p==2 else (255,106,141),width=5)
d.rounded_rectangle((520,893,920,1025),radius=23,fill=(29,8,24),outline=(255,30,58),width=5)
d.rounded_rectangle((980,893,1390,1025),radius=23,fill=(30,22,5),outline=(255,210,29),width=5)
d.text((608,933),'PLAYER 1',font=font(38),fill=(255,74,93))
d.text((1062,933),'PLAYER 2',font=font(38),fill=(255,212,46))
base.save(out/'assets/scene_menu.jpg',quality=91)
# Game image has the identical background/board, with all cells empty.
game=base.copy();d=ImageDraw.Draw(game)
for r in range(6):
 for c in range(7):
    x=625+c*112;y=277+r*99
    d.ellipse((x-43,y-43,x+43,y+43),fill=(2,9,35),outline=(40,133,245),width=8)
game.save(out/'assets/scene_game.jpg',quality=91)
for label,color in [('red',(238,25,66)),('yellow',(255,202,35))]:
    chip=Image.new('RGBA',(104,104),(0,0,0,0));p=ImageDraw.Draw(chip)
    p.ellipse((5,5,99,99),fill=(*color,255),outline=(255,231,191,255) if label=='yellow' else (255,98,127,255),width=6)
    p.ellipse((17,15,83,80),outline=(255,255,255,90),width=3)
    (out/'assets'/f'{label}.rgba').write_bytes(chip.tobytes())
    chip.save(out/'assets'/f'{label}.png')
logo=base.crop((10,0,545,535)).resize((512,512))
logo.save(out/'sce_sys/icon0.png')
print('Generated 4 WINS scene and game assets.')
