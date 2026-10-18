#!/usr/bin/env python3
"""Bake a UTF-8 Latin UI SDF atlas in PS5-DevBridge .huifont v1 format.

This generated atlas is a derivative of DejaVu Sans (Bitstream Vera terms).
The build uses the Debian fonts-dejavu-core package and includes its licence.
Only required glyphs are baked: no raw TTF is distributed.
"""
import argparse
import pathlib
import struct
import unicodedata
from PIL import Image, ImageDraw, ImageFont

CHAR_SIZE = 60
SDF_RANGE = 8.0
PADDING = 10
ATLAS_WIDTH = 2048
HEADER = "<IIHHfffffII"
GLYPH = "<IHHHHfff"
assert struct.calcsize(HEADER) == 40
assert struct.calcsize(GLYPH) == 24

def sdf(mask):
    width, height=mask.size
    pix=mask.load()
    data=[pix[x,y] >= 128 for y in range(height) for x in range(width)]
    dist=[9999.0]*len(data)
    for y in range(height):
        for x in range(width):
            i=y*width+x
            if any(data[i]!=data[(y+dy)*width+x+dx] for dx,dy in
                   ((0,-1),(0,1),(-1,0),(1,0))
                   if 0<=x+dx<width and 0<=y+dy<height):
                dist[i]=0.5
    root2=1.41421356237
    for y in range(height):
        for x in range(width):
            i=y*width+x
            if x:dist[i]=min(dist[i],dist[i-1]+1)
            if y:dist[i]=min(dist[i],dist[i-width]+1)
            if x and y:dist[i]=min(dist[i],dist[i-width-1]+root2)
            if x+1<width and y:dist[i]=min(dist[i],dist[i-width+1]+root2)
    for y in range(height-1,-1,-1):
        for x in range(width-1,-1,-1):
            i=y*width+x
            if x+1<width:dist[i]=min(dist[i],dist[i+1]+1)
            if y+1<height:dist[i]=min(dist[i],dist[i+width]+1)
            if x+1<width and y+1<height:dist[i]=min(dist[i],dist[i+width+1]+root2)
            if x and y+1<height:dist[i]=min(dist[i],dist[i+width-1]+root2)
    out=Image.new("L",mask.size)
    result=out.load()
    for y in range(height):
        for x in range(width):
            i=y*width+x
            v=128+(1 if data[i] else -1)*dist[i]*128/SDF_RANGE
            result[x,y]=max(0,min(255,round(v)))
    return out

def create(out, texts, fontfile):
    chars=set(chr(c) for c in range(32,127))
    chars.update("—–•…←↑→↓▲▶▼◀✓ ÄäÖöÜüß ÉéÈèÀàÂâÊêÎîÔôÙùÛûËëÏïÇçÑñÍíÓóÚúŒœ¿¡")
    for path in texts:
        chars.update(unicodedata.normalize("NFC",pathlib.Path(path).read_text(encoding="utf-8")))
    chars={ch for ch in chars if ch.isprintable()}
    font=ImageFont.truetype(str(fontfile),CHAR_SIZE)
    ascent,descent=font.getmetrics()
    cells=[]
    for ch in sorted(chars):
        code=ord(ch)
        bb=font.getbbox(ch,anchor="ls")
        if bb is None:continue
        bx,by,ex,ey=bb
        gw=max(0,ex-bx);gh=max(0,ey-by)
        advance=float(font.getlength(ch))
        if gw==0 or gh==0:
            cells.append((code,0,0,0,0,0.0,0.0,advance,None))
            continue
        w=gw+PADDING*2;h=gh+PADDING*2
        if w>ATLAS_WIDTH or h>300:raise ValueError("Oversize glyph")
        mask=Image.new("L",(w,h),0)
        ImageDraw.Draw(mask).text((PADDING-bx,PADDING-by),ch,font=font,fill=255,anchor="ls")
        if not mask.getbbox():raise ValueError(f"Missing font glyph: U+{code:04X}")
        cells.append((code,w,h,float(bx-PADDING),float(by-PADDING),advance,sdf(mask)))
    # Row pack glyph SDFs (atlas expanded to exact used height).
    records=[];at_x=0;at_y=0;row_h=0
    for item in cells:
        if len(item)==9:
            cp,_,_,_,_,ox,oy,advance,_=item
            records.append((cp,0,0,0,0,ox,oy,advance,None));continue
        cp,w,h,ox,oy,advance,img=item
        if at_x+w>ATLAS_WIDTH:
            at_x=0;at_y+=row_h+2;row_h=0
        records.append((cp,at_x,at_y,w,h,ox,oy,advance,img))
        at_x+=w+2
        row_h=max(row_h,h)
    height=at_y+row_h+2
    if height>=65535:raise ValueError("Atlas height exceeds huifont format")
    atlas=Image.new("L",(ATLAS_WIDTH,height),0)
    glyph_bytes=bytearray()
    for cp,x,y,w,h,ox,oy,advance,img in records:
        if img:atlas.paste(img,(x,y))
        glyph_bytes+=struct.pack(GLYPH,cp,x,y,w,h,ox,oy,advance)
    header=struct.pack(HEADER,0x46505A50,1,ATLAS_WIDTH,height,
                        float(CHAR_SIZE),SDF_RANGE,float(ascent),-float(descent),
                        float(10),len(records),0)
    output=pathlib.Path(out);output.parent.mkdir(parents=True,exist_ok=True)
    output.write_bytes(header+glyph_bytes+atlas.tobytes())
    print(f"Glyphs={len(records)} atlas={ATLAS_WIDTH}x{height} bytes={output.stat().st_size}")
    for ch in "ÄäÖöÜüßÉéÑñÇç¿¡ŒœЁёБбЯяЖжШшЪъЫы":
        if ord(ch) not in {r[0] for r in records}:
            raise ValueError(f"Required Western-language glyph absent: {ch}")

if __name__=="__main__":
    p=argparse.ArgumentParser()
    p.add_argument("output",type=pathlib.Path)
    p.add_argument("inputs",nargs="+",type=pathlib.Path)
    p.add_argument("--ttf",default="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")
    a=p.parse_args()
    create(a.output,a.inputs,pathlib.Path(a.ttf))
