#!/usr/bin/env python3
"""Derive demo-1 Sunshine tube and ring route from authentic COLLLISION masks.

This is a *native PS5 preview cache* of the original geometry, not the full
4-mode physicsactor.c implementation. We keep an explicit safety condition:
every generated route point must be in free mask space.
"""
from __future__ import annotations
import math
import sys
from collections import defaultdict
from pathlib import Path

def main(app:Path)->None:
    shared=app/"assets"/"level-preview"
    stage=app/"assets"/"stages"/"demo-1"
    header=(shared/"header.txt").read_text().splitlines()
    aw,ah=map(int,header[0].split()[1:])
    mask=(shared/"collision.bin").read_bytes()
    assert len(mask)==aw*ah and aw==1024
    defs={}
    for row in (shared/"defs.txt").read_text().splitlines():
        id,sx,sy,w,h,kind,*rest=row.split()
        defs[int(id)]=(int(sx),int(sy),int(w),int(h),kind)
    bricks=[]
    for row in (shared/"bricks.txt").read_text().splitlines():
        args=row.split()
        id,x,y=int(args[0]),int(args[1]),int(args[2])
        bricks.append((id,x,y,"hflip" in args[3:]))
    assert any(id==50 and x==3712 and y==9984 for id,x,y,_ in bricks)
    assert any(id==41 and x==5760 and y==9664 for id,x,y,_ in bricks)
    assert any(id==42 and x==5888 and y==9664 for id,x,y,_ in bricks)
    cells=defaultdict(list)
    for id,x,y,flip in bricks:
        sx,sy,w,h,typ=defs[id]
        if typ!="SOLID":continue
        if not (x<=6200 and x+w>=3600 and y<=10400 and y+h>=9500):
            continue
        for gx in range(x//128,(x+w-1)//128+1):
            for gy in range(y//128,(y+h-1)//128+1):
                cells[(gx,gy)].append((id,x,y,flip))

    def solid(x:float,y:float)->bool:
        px=math.floor(x);py=math.floor(y)
        for id,bx,by,flip in cells.get((px//128,py//128),()):
            sx,sy,w,h,_=defs[id]
            rx=px-bx;ry=py-by
            if 0<=rx<w and 0<=ry<h:
                if flip:rx=w-1-rx
                if mask[(sy+ry)*aw+(sx+rx)]!=0:return True
        return False

    snake=[]
    missing=[]
    for x in range(3704,4481,4):
        # Find every air corridor BETWEEN TWO occupied mask surfaces.
        y0,y1=9970,10320
        gaps=[];opened=None
        for y in range(y0,y1+2):
            free=y<=y1 and not solid(x,y)
            if free and opened is None:opened=y
            if not free and opened is not None:
                stop=y-1
                if opened>y0 and stop<y1 and stop-opened+1>=26:
                    gaps.append((opened,stop))
                opened=None
        # Author's real two-tile wave: upper gap ~10032..10060 at x3712,
        # lower gap ~10160..10200 at x3840, repeated every 256px.
        expected_center=10122-85*math.cos(math.tau*(x-3712)/256)
        options=[(abs((a+b)*.5-expected_center),a,b) for a,b in gaps]
        if not options:
            snake.append((x,None));missing.append(x);continue
        _,top,bottom=min(options)
        # Native rolling probes cover foot-25..foot-8.
        # Put those probes IN the original free passage, directly above its
        # bottom surface. This is physical geometry, not a graphic sinusoid.
        feet=bottom+8
        assert not any(solid(x,feet-h) for h in (25,16,8)), (x,top,bottom)
        snake.append((x,float(feet)))
    if len(missing)>14:
        raise AssertionError(f"Too many missing original snake mask samples: {missing[:30]}")
    # Fill isolated seams only if both neighbouring real samples exist.
    for i,(x,y) in enumerate(snake):
        if y is not None:continue
        left=next((j for j in range(i-1,-1,-1) if snake[j][1] is not None),None)
        right=next((j for j in range(i+1,len(snake)) if snake[j][1] is not None),None)
        if left is None or right is None or right-left>5:
            raise AssertionError(f"Gap across unsupported snake collision x={x}")
        ly=snake[left][1];ry=snake[right][1]
        snake[i]=(x,ly+(ry-ly)*(i-left)/(right-left))
    assert 90<max(y for _,y in snake)-min(y for _,y in snake)<260

    ring=[]
    cx,cy=5888.,9824.
    radii=[]
    samples=256
    for i in range(samples+1):
        theta=math.pi*.5-math.tau*i/samples
        ux,uy=math.cos(theta),math.sin(theta)
        if solid(cx+ux*36.,cy+uy*36.):
            raise AssertionError("Loop interior is not open at radius 36")
        boundary=next((r for r in range(48,116)
                       if solid(cx+r*ux,cy+r*uy)),None)
        if boundary is None:
            raise AssertionError(f"Original ring outer collision boundary absent: {i}")
        # Roll centre is 18 px INSIDE the original ring wall. A 29 px-tall
        # rolling sprite must not be centered on the opaque wall itself.
        radius=boundary-19
        if radius<45 or radius>95:
            raise AssertionError(f"Suspicious original ring radius={radius} at {i}")
        px=cx+radius*ux;py=cy+radius*uy
        if solid(px,py):
            raise AssertionError(f"Ring path is inside original collision at {i}")
        ring.append((px,py))
        radii.append(radius)
    assert 256==len(ring)-1

    out="".join(f"snake\t{x}\t{y:.2f}\n" for x,y in snake)
    out+="".join(f"ring\t{x:.2f}\t{y:.2f}\n" for x,y in ring)
    for dest in (shared,stage):
        dest.mkdir(parents=True,exist_ok=True)
        (dest/"curves.tsv").write_text(out,encoding="ascii")
    print("ORIGINAL_CURVES_CACHE snake_points",len(snake),
          "snake_feet",round(min(y for _,y in snake),2),
          round(max(y for _,y in snake),2),
          "missing_interpolated",len(missing),
          "ring_points",len(ring),
          "original_inner_ring_radius",min(radii),max(radii))

if __name__=="__main__":
    if len(sys.argv)!=2:raise SystemExit("Usage: prepare-curves-preview.py <app>")
    main(Path(sys.argv[1]))
