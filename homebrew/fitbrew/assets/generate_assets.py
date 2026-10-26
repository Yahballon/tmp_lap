#!/usr/bin/env python3
"""Stage FITBREW assets. Prefer checked-in original photo art; generate an honest
offline vector-style fallback if optional image files are absent."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter
import math, shutil, sys
target=Path(sys.argv[1]); root=Path(__file__).resolve().parent
(target/"assets").mkdir(parents=True,exist_ok=True)
(target/"sce_sys").mkdir(parents=True,exist_ok=True)
# Each pose is a separate runtime JPEG to keep texture budgets bounded.
def draw_pose(i):
    sz=512
    im=Image.new("RGB",(sz,sz),(9,14,21))
    d=ImageDraw.Draw(im)
    for y in range(sz):
        v=int(y*0.031)
        d.line((0,y,sz,y),fill=(9+v//3,14+v//4,21+v//5))
    for x in (64,192,364,474):
        d.line((x,0,x-30,512),fill=(31,40,48),width=6)
    for x in (48,254,442):
        d.rounded_rectangle((x,110,x+8,356),radius=4,fill=(165,218,17))
    cx=260+int(math.sin(i*2.1)*46); cy=212+int(math.cos(i)*18)
    skin=(182,121,83) if i%2==0 else (145,100,79)
    # Stylized original pose illustration, with independent drawn geometry.
    d.ellipse((cx-45,cy-118,cx+45,cy-28),fill=skin)
    d.ellipse((cx-48,cy-123,cx+47,cy-69),fill=(23,23,26))
    torso=(24,30,35)
    d.rounded_rectangle((cx-73,cy-27,cx+72,cy+157),radius=40,fill=torso)
    xshift=(-35,12,47,-45,30,-30)[i]
    d.line((cx-65,cy+20,cx-117+xshift,cy+160,cx-155+xshift,cy+210),fill=skin,width=30,joint="curve")
    d.line((cx+65,cy+20,cx+115-xshift,cy+146,cx+151-xshift,cy+211),fill=skin,width=30,joint="curve")
    d.line((cx-42,cy+141,cx-84+xshift,cy+271,cx-98+xshift,cy+355),fill=(42,49,55),width=48,joint="curve")
    d.line((cx+42,cy+141,cx+110-xshift,cy+262,cx+122-xshift,cy+355),fill=(42,49,55),width=48,joint="curve")
    # Vignette keeps text legible on top.
    overlay=Image.new("RGBA",(sz,sz)); od=ImageDraw.Draw(overlay)
    for y in range(200):
        alpha=int(130*(1-y/200))
        od.line((0,y,512,y),fill=(5,9,15,alpha))
    im=Image.alpha_composite(im.convert("RGBA"),overlay).convert("RGB")
    return im
for i in range(6):
    dest=target/"assets"/f"pose_{i}.jpg"
    source=root/f"pose_{i}.jpg"
    if source.is_file():
        im=Image.open(source).convert("RGB").resize((512,512))
    else:
        im=draw_pose(i)
    im.save(dest,quality=88,optimize=True)
icon_source=root/"icon-ai.png"
if icon_source.is_file():
    icon=Image.open(icon_source).convert("RGB").resize((512,512))
else:
    icon=draw_pose(0)
    d=ImageDraw.Draw(icon)
    d.rounded_rectangle((12,335,500,497),radius=22,fill=(6,10,16))
    d.text((45,350),"FITBREW",fill=(215,255,0),stroke_width=0)
    d.line((42,422,472,422),fill=(215,255,0),width=6)
icon.save(target/"sce_sys"/"icon0.png",optimize=True)
print("FITBREW artwork staged: six 512x512 JPEG workout photographs/illustrations and launcher icon")
