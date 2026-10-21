#!/usr/bin/env python3
"""Stage exact 4 WINS artwork from attached reference; generate only dynamic chips.

The supplied photo already has an empty, correctly proportioned Connect Four grid.
Live menus, active columns and tokens are rendered through ps5-homebrew-ui.
"""
from PIL import Image, ImageDraw, ImageFilter
from pathlib import Path
import shutil, sys

root=Path(__file__).resolve().parent
out=Path(sys.argv[1])
(out/'assets').mkdir(parents=True,exist_ok=True)
(out/'sce_sys').mkdir(parents=True,exist_ok=True)
src=root/'scene_game.jpg'
if not src.is_file(): raise RuntimeError('Missing real scene_game.jpg artwork')
with Image.open(src) as image:
    assert image.width>=1600 and image.height>=900, image.size
    image=image.convert('RGB')
    # Native virtual coordinates are 1920x1080; use predictable full-screen texels.
    image=image.resize((1920,1080),Image.Resampling.LANCZOS)
    # Original photo is a mock-up: all old button/score text is embedded.
    # Replace those pixels with quiet, dark matte staging areas, leaving
    # the true board, 4 WINS logo, table chips and furniture unchanged.
    # Live interactive UI will fully cover these neutral matte areas.
    from PIL import ImageDraw
    matte=ImageDraw.Draw(image)
    matte.rounded_rectangle((49,299,477,706),radius=20,fill=(4,11,32))
    matte.rounded_rectangle((484,875,944,1029),radius=22,fill=(5,9,22))
    matte.rounded_rectangle((982,875,1444,1029),radius=22,fill=(5,9,22))
    image.save(out/'assets/scene_game.jpg',quality=94,optimize=True,subsampling=0)
    # Original supplied game's logo, not the mismatching v0.1.0 procedural title.
    icon=image.crop((30,14,732,430)).resize((512,512),Image.Resampling.LANCZOS)
    icon.save(out/'sce_sys/icon0.png',optimize=True)

# Soft gloss + outer rim without copying any part of the background into the
# dynamic disc. Alpha8 prevents rectangles on the 7 x 6 holes.
S=96
for name,base,edge in [('red',(247,18,59),(255,90,112)),('yellow',(255,205,27),(255,250,135))]:
    disc=Image.new('RGBA',(S,S),(0,0,0,0));p=ImageDraw.Draw(disc)
    for radius in range(45,0,-1):
        t=(45-radius)/45
        col=tuple(max(0,min(255,int(v*(1-t*.19)))) for v in base)
        p.ellipse((48-radius,48-radius,48+radius,48+radius),fill=(*col,255))
    p.ellipse((3,3,93,93),outline=(*edge,255),width=5)
    p.ellipse((10,9,86,84),outline=(255,255,255,95),width=3)
    glow=Image.new('RGBA',(S,S),(0,0,0,0));g=ImageDraw.Draw(glow)
    g.ellipse((20,12,70,30),fill=(255,255,255,80))
    glow=glow.filter(ImageFilter.GaussianBlur(7))
    disc=Image.alpha_composite(disc,glow)
    (out/'assets'/f'{name}.rgba').write_bytes(disc.tobytes())
    disc.save(out/'assets'/f'{name}.png',optimize=True)
print('PASS: photoreal 4 WINS scene (1920x1080), transparent discs (96x96), icon (512x512)')
