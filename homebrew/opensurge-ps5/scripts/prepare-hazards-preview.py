#!/usr/bin/env python3
"""Stage original demo-1 enemy/hazard positions and pixel art for native PS5.
Conservative compatibility subset; does NOT claim to execute SurgeScript.
"""
from pathlib import Path
from PIL import Image
import re
import sys
from collections import Counter

TYPES={"Mosquito":"mosquito","Wolfey":"wolfey","Lady Bugsy":"bugsy","Spikes":"spikes"}
ART={"mosquito":"images/mosquito.png","wolfey":"images/wolfey.png",
     "bugsy":"images/bugsy.png","spikes":"images/spikes.png"}
PAT=re.compile(r'(?m)^\s*entity\s+"([^"]{1,80})"\s+(-?\d+)\s+(-?\d+)\s+"[0-9a-f]+"')
EXPECTED={"mosquito":5,"wolfey":1,"bugsy":1,"spikes":6}

def build(original:Path,app:Path)->None:
    source=original/"game"
    level=(source/"levels/demo-1.lev").read_text(encoding="utf-8")
    positions=[]
    for name,xs,ys in PAT.findall(level):
        if name not in TYPES: continue
        x,y=int(xs),int(ys)
        if not(-100000<x<100000 and -100000<y<100000):raise ValueError("entity coordinate invalid")
        positions.append((TYPES[name],x,y))
    counts=Counter(kind for kind,_,_ in positions)
    if counts!=EXPECTED:raise ValueError(f"Unexpected original hazard count: {counts}")
    dest=app/"assets"/"level-preview"
    dest.mkdir(parents=True,exist_ok=True)
    (dest/"hazards.tsv").write_text("".join(f"{k}\t{x}\t{y}\n" for k,x,y in positions),encoding="ascii")
    for kind,path in ART.items():
        with Image.open(source/path) as pic:
            out=pic.convert("RGBA")
            w,h=out.size
            if not(100<=w<=4096 and 32<=h<=4096):raise ValueError(f"bad sheet dimensions {kind}")
            pixels=bytearray(out.tobytes())
        for i in range(0,len(pixels),4):
            if pixels[i:i+3]==bytes((255,0,255)) or pixels[i+3]==0:
                pixels[i:i+4]=bytes((0,0,0,0))
        (dest/(kind+".rgba")).write_bytes(pixels)
        (dest/(kind+".txt")).write_text(f"{w} {h}\n",encoding="ascii")
    print("ORIGINAL_HAZARDS",dict(counts),len(positions))

if __name__=="__main__":
    if len(sys.argv)!=3:raise SystemExit("Usage: prepare-hazards-preview.py <legacy-stage> <app>")
    build(Path(sys.argv[1]),Path(sys.argv[2]))
