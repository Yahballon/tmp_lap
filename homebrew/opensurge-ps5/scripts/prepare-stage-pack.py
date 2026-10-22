#!/usr/bin/env python3
"""Package four ORIGINAL, pinned, Sunshine-theme levels for native PS5 testing.

The original upstream level files are shipped separately under assets/game.
This is ONLY a native compatibility cache: original SurgeScript objects, loops,
camera, and special physics are not executed. Never fabricate an object type.
"""
from pathlib import Path
import re
import sys
from collections import Counter

STAGES=("demo-1", "sunshine-1", "sunshine-2", "sunshine-3")
ENTITY_KINDS={
    "Collectible":"ring",
    "Spring Standard":"spring",
    "Spring Stronger":"spring_stronger",
    "Goal":"goal",
    "Goal Capsule":"goal", # end trigger compatibility only, NOT original capsule logic
    "Checkpoint":"checkpoint",
}
HAZARDS={
    "Mosquito":"mosquito", "Wolfey":"wolfey",
    "Lady Bugsy":"bugsy", "Spikes":"spikes",
}
OBJ=re.compile(r'^\s*entity\s+"([^"]+)"\s+(-?\d+)\s+(-?\d+)\s+"([0-9a-f]+)"\s*$')
BRICK=re.compile(r'^\s*brick\s+(\d+)\s+(-?\d+)\s+(-?\d+)(?:\s+(green|yellow))?\s*$')
SPAWN=re.compile(r'^\s*spawn_point\s+(-?\d+)\s+(-?\d+)\s*$')
THEME=re.compile(r'^\s*theme\s+"([^"]+)"')
MUSIC=re.compile(r'^\s*music\s+"([^"]+)"')

def run(staged:Path,app:Path)->None:
    root=staged/"game"
    shared=app/"assets"/"level-preview"
    cache=app/"assets"/"stages"
    assert (shared/"defs.txt").is_file(), "Prepare original demo first"
    defs=shared.joinpath("defs.txt").read_text(encoding="ascii")
    depth={}
    for row in defs.splitlines():
        fields=row.split()
        depth[int(fields[0])]=float(fields[6])
    assert len(depth)>=90
    common_header=(shared/"header.txt").read_text(encoding="ascii")
    atlas=re.search(r"^atlas (\d+) (\d+)$",common_header,re.M)
    hero=re.search(r"^hero (\d+) (\d+)$",common_header,re.M)
    assert atlas and hero
    for stage in STAGES:
        source=root/"levels"/(stage+".lev")
        text=source.read_text(encoding="utf-8")
        spawn=next((m for line in text.splitlines() if (m:=SPAWN.match(line))),None)
        theme=next((m.group(1) for line in text.splitlines() if (m:=THEME.match(line))),None)
        music=next((m.group(1) for line in text.splitlines() if (m:=MUSIC.match(line))),None)
        if spawn is None or theme!="themes/sunshine.brk":
            raise ValueError(f"{stage}: unsupported original theme/spawn; refusing fake assets")
        bricks=[];entities=[];hazards=[]; routes=[]
        for line in text.splitlines():
            if m:=BRICK.match(line):
                ident,x,y=int(m[1]),int(m[2]),int(m[3])
                if ident not in depth or abs(x)>100000 or abs(y)>100000:
                    raise ValueError(f"{stage}: unsupported brick {ident}")
                bricks.append((ident,x,y,m[4] or "default"))
            elif m:=OBJ.match(line):
                name,x,y=m[1],int(m[2]),int(m[3])
                if abs(x)>100000 or abs(y)>100000:raise ValueError("bad entity coordinate")
                if name in ENTITY_KINDS: entities.append((ENTITY_KINDS[name],x,y))
                if name in HAZARDS: hazards.append((HAZARDS[name],x,y))
                if name in ("Layer Green","Layer Yellow","Tube In","Tube Out"):
                    kind={"Layer Green":"green","Layer Yellow":"yellow",
                          "Tube In":"tube_in","Tube Out":"tube_out"}[name]
                    routes.append((kind,x,y))
        if len(bricks)<500 or len(entities)<15 or not any(x[0]=="goal" for x in entities):
            raise ValueError(f"{stage}: original authored stage requires different engine")
        bricks.sort(key=lambda x:(depth[x[0]],x[2],x[1]))
        d=cache/stage
        d.mkdir(parents=True,exist_ok=True)
        (d/"header.txt").write_text(
            f"atlas {atlas[1]} {atlas[2]}\n"
            f"hero {hero[1]} {hero[2]}\n"
            f"spawn {spawn[1]} {spawn[2]}\n"
            f"instances {len(bricks)}\n",encoding="ascii")
        (d/"defs.txt").write_text(defs,encoding="ascii")
        (d/"bricks.txt").write_text(
            "".join(f"{a} {b} {c} {layer}\n" for a,b,c,layer in bricks),encoding="ascii")
        for target,items in (("routes.tsv",routes),("entities.tsv",entities),("hazards.tsv",hazards)):
            (d/target).write_text(
                "".join(f"{a}\t{b}\t{c}\n" for a,b,c in items),encoding="ascii")
        (d/"music.txt").write_text((music or "musics/template.ogg")+"\n",encoding="utf-8")
        print("STAGE_PACK",stage,"bricks",len(bricks),"entities",len(entities),
              "hazards",len(hazards),"routes",len(routes),"music",music)
    (cache/"catalog.tsv").write_text(
        "".join(f"{stage}\t{(cache/stage/'music.txt').read_text().strip()}\n"
                for stage in STAGES),encoding="ascii")
if __name__=="__main__":
    if len(sys.argv)!=3:raise SystemExit("prepare-stage-pack.py <staged> <app>")
    run(Path(sys.argv[1]),Path(sys.argv[2]))
