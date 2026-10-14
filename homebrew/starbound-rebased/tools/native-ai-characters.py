#!/usr/bin/env python3
"""Place user HD character art in the ORIGINAL Gold872 1x frame layout.

The engine uses (1+16*col, 9+16*row) for the player and
(1+16*col, 1+16*row) for enemies. A powered-up player is ONE
16x32 sprite, not two 16x16 sprites.
"""
from pathlib import Path
from PIL import Image, ImageDraw

root=Path(__file__).resolve().parents[1]
atlas=root/"res/sprites/characters"
source=root/"hd-assets/characters"
player=Image.open(atlas/"PlayerSpriteSheet.png").convert("RGBA")
enemy=Image.open(atlas/"EnemySpriteSheet.png").convert("RGBA")
hero=Image.open(source/"PlayerSpriteSheet.png").convert("RGBA")
foes=Image.open(source/"EnemySpriteSheet.png").convert("RGBA")
assert player.size==(403,266) and enemy.size==(562,242)
assert hero.size==(806,532) and foes.size==(1120,480)
nearest=Image.Resampling.NEAREST

def crop_hero(col, group=0):
    x=2+(col%25)*32
    y=18+group*32
    img=hero.crop((x,y,x+32,y+32))
    assert img.getchannel("A").getbbox()
    return img

def frame(col, group=0, big=False):
    im=crop_hero(col,group)
    if not big:
        return im.resize((16,16),nearest)
    # Stretch suit/body separately so one large model is rendered.
    out=Image.new("RGBA",(16,32))
    out.paste(im.crop((0,0,32,21)).resize((16,17),nearest),(0,0))
    out.paste(im.crop((0,21,32,32)).resize((16,15),nearest),(0,17))
    return out

def replace(image, x, y, sprite, clear=(16,16)):
    assert 0<=x and x+clear[0]<=image.width
    assert 0<=y and y+clear[1]<=image.height
    image.paste((0,0,0,0),(x,y,x+clear[0],y+clear[1]))
    image.paste(sprite,(x,y))

# Small astronaut and fire astronaut: actual user-supplied art.
for row,group in ((0,0),(8,3)):
    for col in range(25):
        replace(player,1+16*col,9+16*row,frame(col,group))

# Large forms span exactly two original rows. This eliminates the
# two-small-astronauts stack when a power-up is collected.
for row,group,end in ((1,0,25),(9,3,21)):
    for col in range(end):
        x,y=1+16*col,9+16*row
        if col in (5,6): # crouch/collapse frames use 16-high sourceRects
            replace(player,x,y,frame(col,group),clear=(16,32))
        else:
            replace(player,x,y,frame(col,group,True),clear=(16,32))

# IDs 246/247 = projectile and impact; source is 8x8, NEVER a character.
for col,impact in ((21,False),(22,True)):
    x,y=1+16*col,9+16*9
    sprite=Image.new("RGBA",(16,16))
    d=ImageDraw.Draw(sprite)
    if impact:
        d.polygon([(8,0),(10,5),(15,4),(12,9),(14,14),(8,11),
                   (2,14),(4,9),(1,4),(6,5)],fill="#1bffe1",
                  outline="#07152f")
    else:
        d.ellipse((1,1,13,13),fill="#03e8e3",outline="#07152f",width=2)
        d.ellipse((5,3,8,6),fill="#ffffff")
    replace(player,x,y,sprite,clear=(16,32))

# Enemy IDs 70/71 are small slimes on reference World1-1.
for col in (0,1):
    ai=foes.crop((col*32,0,col*32+32,32)).resize((16,16),nearest)
    assert ai.getchannel("A").getbbox()
    replace(enemy,1+16*col,1+2*16,ai)

# IDs 38/39 use 16x32 armored crawler sprite source regions.
for col in (3,4):
    ai=foes.crop((col*32,32,col*32+32,96)).resize((16,32),nearest)
    assert ai.getchannel("A").getbbox()
    replace(enemy,1+16*col,1+16,ai,clear=(16,32))

player.save(atlas/"PlayerSpriteSheet.png",optimize=True)
enemy.save(atlas/"EnemySpriteSheet.png",optimize=True)
assert Image.open(atlas/"PlayerSpriteSheet.png").size==(403,266)
assert Image.open(atlas/"EnemySpriteSheet.png").size==(562,242)
# Fireball draw frames have less coverage than a whole humanoid.
p=player.crop((337,153,345,161))
assert p.getchannel("A").getbbox()
# Big sprite must be one 16x32 continuous figure.
p=player.crop((113,25,129,57))
a=p.getchannel("A")
assert a.crop((0,0,16,16)).getbbox() and a.crop((0,16,16,32)).getbbox()
print("USER AI SPRITES: small 16x16, power-up 16x32, cyan projectile IDs, enemy IDs OK")