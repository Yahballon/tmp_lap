#!/usr/bin/env python3
"""Original, procedural HD character/tile asset atlas for Starbound Sprint.
Every 64x64 RGBA cell is authored here; no Nintendo sprites or external art.
Pillow supersampling ensures smooth silhouette edges on 1080p screens.
"""
from PIL import Image,ImageDraw,ImageFilter
from pathlib import Path
import math,random
random.seed(2607)
CELL=64
S=4
atlas=Image.new("RGBA",(512,512))
def tile(index,kind):
    image=Image.new("RGBA",(CELL*S,CELL*S))
    d=ImageDraw.Draw(image)
    def e(box,fill,outline=None,width=1):
        d.ellipse(tuple(int(v*S) for v in box),fill=fill,outline=outline,width=width*S)
    def r(box,fill,radius=0):
        d.rounded_rectangle(tuple(int(v*S) for v in box),radius=radius*S,fill=fill)
    def line(points,fill,width=2):
        d.line([(int(x*S),int(y*S)) for x,y in points],fill=fill,width=width*S,joint="curve")
    def poly(points,fill):
        d.polygon([(int(x*S),int(y*S)) for x,y in points],fill=fill)
    if kind=="hero":
        action=index
        step=math.sin(action*math.pi/4)
        # original astral courier: cobalt-blue suit, teal scarf, floating silver hair.
        poly([(23,34),(7+step*3,31),(4,43),(30,43)],"#3be8cf")
        poly([(25,36),(7,32),(2,45),(23,41)],"#128bba")
        line([(26,44),(19+step*5,58)],"#0d1939",7)
        line([(37,44),(45-step*5,58)],"#142b50",7)
        r([14+step*5,54,27+step*5,61],"#17254a",4)
        r([40-step*5,54,55-step*5,61],"#17254a",4)
        r([20,27,46,51],"#101d42",7)
        poly([(27,31),(41,31),(43,47),(29,47)],"#244e83")
        poly([(20,30),(29,28),(20,42)],"#173765")
        poly([(45,31),(52,36),(47,44)],"#173765")
        e((29,34,39,44),"#08e8f1")
        e((32,36,37,41),"#d8ffff")
        e((18,6,46,33),"#ffe0b8")
        # side swept unmistakably different asymmetric cut
        poly([(17,19),(11,17),(21,9),(16,2),(32,8),(39,4),(51,11),(43,14),(48,20),(34,16),(25,21)],"#e4f2ff")
        poly([(15,16),(22,6),(32,12),(46,9),(38,16)],"#a5d6ef")
        e((36,18,42,23),"#183b5d")
        e((38,18,40,20),"#e4ffff")
        if action==6:
            poly([(28,51),(39,51),(32,63)],"#4de6ff")
        if action==7:
            line([(29,54),(17,60)],"#1a2c57",6)
    elif kind=="enemy":
        colors=["#e86b8a","#77dfa5","#fbc16a","#7ba4ff","#cb7bf0","#69cce9","#ff9670","#8cd6a6"]
        c=colors[index%8]
        e((8,14,57,57),"#111d3a")
        e((11,10,54,52),c)
        poly([(18,17),(9,6),(32,15)],c)
        poly([(46,18),(55,5),(35,14)],c)
        e((21,23,30,35),"#f6faff")
        e((36,23,45,35),"#f6faff")
        e((25,26,29,33),"#101e43")
        e((36,26,40,33),"#101e43")
        poly([(24,42),(31,46),(38,42)],"#351e46")
        for j in range(3):
            line([(18+j*14,50),(16+j*14,58)],"#1d2b4b",3)
        if index in (3,4,7):
            e((4,31,16,41),"#f8f3a4")
            e((49,31,61,41),"#f8f3a4")
    elif kind=="pickup":
        hues=["#46dbff","#58ffbe","#e79fff","#ffb65d","#8fcfff","#ffffa2","#ff8abb","#9de8fc"]
        c=hues[index%8]
        poly([(32,4),(49,27),(34,58),(16,29)],c)
        poly([(32,4),(34,58),(16,29)],"#70f8ff" if index%2==0 else "#91edc7")
        poly([(32,4),(49,27),(34,58),(36,27)],"#f2ffff")
        line([(31,8),(30,48)],"#ffffff",2)
    elif kind=="terrain":
        biome=index//8
        hues=[("#385677","#83da97","#c8f7a8"),("#344875","#61e6ff","#b7ffff"),("#664362","#ffa77b","#ffe6ac"),("#53588c","#bba4ff","#e1d7ff")]
        stone,grass,light=hues[biome]
        r([0,0,63,63],stone,3)
        for n in range(8):
            x=(index*17+n*23)%56
            y=(index*13+n*29)%51+9
            poly([(x,y),(x+11,y+2),(x+7,y+12)],"#263852")
        if index%8<5:
            r([0,0,63,14],grass,4)
            line([(0,3),(63,3)],light,3)
            for n in range(9):
                x=n*8
                line([(x,10),(x+2,19+n%3*3)],grass,2)
    elif kind=="prop":
        e((14,8,49,44),"#49ddeb")
        e((19,13,44,38),"#c4ffff")
        poly([(31,5),(52,35),(32,62),(12,35)],"#3ac8f0")
        poly([(31,5),(33,57),(12,35)],"#75e1f7")
    image=image.resize((CELL,CELL),Image.Resampling.LANCZOS)
    atlas.paste(image,((index%8)*CELL,(index//8)*CELL))
for i in range(64):
    if i<8:tile(i,"hero")
    elif i<16:tile(i-8,"enemy")
    elif i<24:tile(i-16,"pickup")
    elif i<56:tile(i-24,"terrain")
    else:tile(i-56,"prop")
dest=Path("app/assets/starbound-sprites.rgba")
dest.parent.mkdir(parents=True,exist_ok=True)
dest.write_bytes(atlas.tobytes())
atlas.save("app/assets/starbound-sprites-preview.png")
print("Wrote original 512x512 RGBA sprite atlas",dest.stat().st_size)
