#!/usr/bin/env python3
"""Temporary 1:1 World1-1 mechanics reference with palette-reskinned original atlases.

Keep exactly the original sprite cells/coordinates, animations, transparent
pixels and key color (147,187,236), so Gold872's hard-coded IDs work.
The current reskin is deterministic procedural pixel-color processing,
NOT generative-AI art. This test needs manual rights review before distribution.
"""
from pathlib import Path
from PIL import Image
import colorsys
import shutil
import hashlib
import sys

ref=Path(sys.argv[1] if len(sys.argv)>1 else "build/gold872-reference")/"res"
out=Path(__file__).resolve().parents[1]/"res"
# Exact data, including hidden underground sub-area + portal coordinates.
level_files=[
    f"World1-1_{layer}.csv" for layer in
    ["Above_Foreground","Background","Collectibles",
     "Enemies","Foreground","Underground"]
]+["World1-1.levelproperties"]
for filename in level_files:
    src=ref/"data/World1-1"/filename
    dst=out/"data/World1-1"/filename
    dst.parent.mkdir(parents=True,exist_ok=True)
    shutil.copy2(src,dst)
    assert hashlib.sha256(dst.read_bytes()).digest()==hashlib.sha256(src.read_bytes()).digest()

# The menu terrain also relies on the original, dense atlas ID definitions.
shutil.copy2(ref/"data/MenuBackground/MenuBackground_Background.csv",
             out/"data/MenuBackground/MenuBackground_Background.csv")
shutil.copy2(ref/"sprites/blocks/IrregularReferences.blockmap",
             out/"sprites/blocks/IrregularReferences.blockmap")

atlases=[
    ("blocks/BlockTileSheet.png",48,22),
    ("characters/PlayerSpriteSheet.png",25,16),
    ("characters/EnemySpriteSheet.png",35,15),
]
def shift_rgb(r,g,b):
    # Nearest-neighbour and alpha-preserving palette reskin.
    # Original color key must be untouched (used by SDL_SetColorKey).
    if (r,g,b)==(147,187,236): return r,g,b
    h,s,v=colorsys.rgb_to_hsv(r/255,g/255,b/255)
    if s<0.13:
        # Near-neutral shades become cool steel while preserving luminance.
        return (int(255*v*.77),int(255*v*.91),int(255*v))
    if h<.09 or h>.96:       target_h=.88 # rose -> violet
    elif h<.20:              target_h=.12 # orange -> warm gold
    elif h<.45:              target_h=.47 # greens -> mint/cyan
    elif h<.73:              target_h=.59 # blue -> deep indigo
    else:                    target_h=.76 # violet -> lilac
    rr,gg,bb=colorsys.hsv_to_rgb(target_h,max(.36,s*.85),v)
    return round(rr*255),round(gg*255),round(bb*255)

for filename,cols,rows in atlases:
    src=Image.open(ref/"sprites"/filename).convert("RGBA")
    # Gold872 atlases have a 1px border and 1px gaps: (16+1)*cells + 1.
    # This is why all earlier 768x352 synthetic tile PNGs misaligned.
    assert src.width==cols*17+1 and src.height==rows*17+1, (filename,src.size)
    pixels=list(src.getdata())
    outpix=[]
    for r,g,b,a in pixels:
        if a==0: outpix.append((r,g,b,0))
        else:
            rr,gg,bb=shift_rgb(r,g,b)
            outpix.append((rr,gg,bb,a))
    recolored=Image.new("RGBA",src.size)
    recolored.putdata(outpix)
    # Enlarging the source atlas is safe only if source rectangles are 2x too.
    # upgrade-renderer.py already doubles every SpritesheetComponent rectangle.
    hd=recolored.resize((src.width*2,src.height*2),Image.Resampling.NEAREST)
    dest=out/"sprites"/filename
    dest.parent.mkdir(parents=True,exist_ok=True)
    hd.save(dest,optimize=True)
    assert hd.size==((cols*17+1)*2,(rows*17+1)*2)
    assert hd.getchannel("A").resize(src.size,Image.Resampling.NEAREST).tobytes()==src.getchannel("A").tobytes()
    print("GRID OK",filename,src.size,"=>",hd.size,"cells",cols*rows)

# For debugging scene correctness, the native blue backdrop is restored via a
# C++ patch. This image remains shipped for future selectable skins.
assert (out/"sprites/icons/logo.png").is_file()
print("REFERENCE WORLD 1-1 EXACT LEVEL FILES",len(level_files),"and matching alpha-preserved atlas positions")
