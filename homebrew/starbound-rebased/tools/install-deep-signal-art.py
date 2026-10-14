#!/usr/bin/env python3
"""Pack normalized original enemy frames into exact native sprite rectangles.

Only the new enemy slots are replaced; the repaired player atlas is untouched.
"""
import shutil
from pathlib import Path
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'hd-assets/deep-signal'
SPR=ROOT/'res/sprites'
NEAREST=Image.Resampling.NEAREST
sheet=Image.open(SPR/'characters/EnemySpriteSheet.png').convert('RGBA')
assert sheet.size==(562,242)
def frames(name,size):
    images=[Image.open(SOURCE/name/f'{i:02}.png').convert('RGBA') for i in range(1,5)]
    assert all(im.size==(64,64) for im in images)
    boxes=[im.getchannel('A').point(lambda a:255 if a>64 else 0).getbbox() for im in images]
    assert all(boxes),name
    factor=min(size[0]/max(b[2]-b[0] for b in boxes),size[1]/max(b[3]-b[1] for b in boxes))
    packed=[]
    for im,bbox in zip(images,boxes):
        im=im.crop(bbox)
        scaled=im.resize((max(1,round(im.width*factor)),max(1,round(im.height*factor))),NEAREST)
        canvas=Image.new('RGBA',size)
        canvas.paste(scaled,((size[0]-scaled.width)//2,size[1]-scaled.height))
        packed.append(canvas)
    return packed

def stamp(eid,frame):
    x,y=1+(eid%35)*16,1+(eid//35)*16
    sheet.paste((0,0,0,0),(x,y,x+frame.width,y+frame.height))
    sheet.paste(frame,(x,y))

for name,ids,size in (('ray',(81,82),(16,16)),('jelly',(48,49),(16,32)),
                      ('sentry',(90,91),(16,16)),('sentinel',(61,63,65,67),(32,32))):
    images=frames(name,size)
    for i,eid in enumerate(ids): stamp(eid,images[i])
sheet.save(SPR/'characters/EnemySpriteSheet.png',optimize=True)
for name in ('underwater_observatory','crystal_quarry','reactor_core'):
    target=SPR/'backgrounds'/f'{name}.png'
    shutil.copyfile(SOURCE/f'{name}.png',target)
    assert Image.open(target).size==(800,480)
print('Packed original rays, jelly drones, sentries, four sentinel frames and three biome backdrops')
