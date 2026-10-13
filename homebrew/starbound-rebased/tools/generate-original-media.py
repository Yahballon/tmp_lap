#!/usr/bin/env python3
"""Generate original game media in the exact Gold872 SDL2 data formats.
No upstream Nintendo sprites, maps, music, logos or fonts are downloaded.
"""
from pathlib import Path
from PIL import Image, ImageDraw
import colorsys, math, random, wave, struct, shutil
ROOT=Path(__file__).resolve().parents[1]/"res"
COLORS=["#4ee6cf","#8ad3ff","#fda379","#ba9ef7","#f8d476","#a5eab2","#fc96bf","#7ab8f4"]
def cell(kind,index):
    im=Image.new("RGBA",(16,16))
    d=ImageDraw.Draw(im)
    x,y=0,0
    if kind=="player":
        step=index%5
        palette=COLORS[(index//25)%8]
        # Luminous crystal explorer: face visor, antenna, asymmetrical scarf.
        d.polygon([(4,7),(0,7),(3,11),(8,9)],fill=palette)
        d.rectangle((5,8,11,13),fill="#173e70")
        d.rectangle((6,9,9,11),fill="#3c9ea6")
        d.line((6,13,5+step%3,15),fill="#29416a",width=2)
        d.line((10,13,11-step%3,15),fill="#29416a",width=2)
        d.ellipse((5,2,11,8),fill="#e1f5fd")
        d.rectangle((7,4,12,6),fill="#07254d")
        d.rectangle((10,4,12,5),fill="#42f9f0")
        d.polygon([(5,3),(3,2),(7,1),(8,0),(11,2),(13,2),(11,4)],fill="#d1ebfc")
        d.ellipse((8,10,9,11),fill="#e5ff9a")
    elif kind=="enemy":
        c=COLORS[(index//8+index%4)%8]
        d.ellipse((2,3,14,14),fill="#172845")
        d.ellipse((3,3,13,12),fill=c)
        d.polygon([(4,4),(2,1),(7,3)],fill=c)
        d.polygon([(11,4),(14,1),(9,3)],fill=c)
        d.rectangle((6,6,7,8),fill="#f9ffff")
        d.rectangle((10,6,11,8),fill="#f9ffff")
        d.point((7,7),fill="#102544");d.point((10,7),fill="#102544")
        d.line((5,12,4,15),fill="#132842",width=2)
        d.line((12,12,13,15),fill="#132842",width=2)
    else:
        if index in (101,149):
            d.line((8,0,8,15),fill="#bcdde4",width=2)
            d.rectangle((7,0,10,3),fill="#f6e9a2")
        elif index==152:
            d.polygon([(2,2),(14,4),(2,9)],fill="#51eaff")
        elif 144<=index<=147:
            d.polygon([(8,1),(13,7),(8,15),(3,7)],fill=COLORS[index%8])
            d.line((8,2,8,13),fill="#ffffff",width=1)
        elif index in (192,193,194,195):
            d.rounded_rectangle((0,0,15,15),radius=2,fill="#314c79",outline="#84e7cb")
            d.polygon([(8,2),(13,8),(8,13),(3,8)],fill="#b9ffce")
        else:
            c=COLORS[(index//48)%8]
            d.rectangle((0,0,15,15),fill="#2e445d")
            d.rectangle((1,1,14,14),fill="#45586f")
            d.line((1,1,14,1),fill=c,width=2)
            seed=random.Random(index)
            for i in range(4):
                ox=seed.randint(1,13);oy=seed.randint(3,13)
                d.line((ox,oy,ox+seed.randint(-3,2),oy+2),fill="#20384d",width=1)
    return im
def sheet(name,width,height,kind):
    output=Image.new("RGBA",(width*16,height*16))
    for i in range(width*height):
        output.alpha_composite(cell(kind,i),((i%width)*16,(i//width)*16))
    path=ROOT/"sprites"/name
    path.parent.mkdir(parents=True,exist_ok=True)
    output.save(path,optimize=True)
sheet("characters/PlayerSpriteSheet.png",25,16,"player")
sheet("characters/EnemySpriteSheet.png",35,15,"enemy")
sheet("blocks/BlockTileSheet.png",48,22,"block")
for name,size in [("icons/logo.png",(460,96)),("icons/optionsbackground.png",(800,480)),("icons/optionsinfobackground.png",(800,480)),("icons/windowicon.png",(64,64))]:
    im=Image.new("RGBA",size,(12,23,53,255))
    d=ImageDraw.Draw(im)
    for i in range(0,size[0],20):
        d.ellipse((i,size[1]//2-4,i+3,size[1]//2-1),fill="#43e6e9")
    if size[0]>100:
        d.rounded_rectangle((10,10,size[0]-10,size[1]-10),radius=20,outline="#3ac8d5",width=3)
    out=ROOT/"sprites"/name
    out.parent.mkdir(parents=True,exist_ok=True)
    if name=="icons/logo.png":
        from PIL import ImageFont
        font=ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",33)
        d.text((30,29),"STARBOUND REBASED",fill="#dffbff",font=font)
    im.save(out)
# Sound is newly synthesized with an independent melody, not copied from game ROMs.
effects="blockbreak blockhit bowserfall bowserfire cannonfire castleclear coin death fireball flagraise gameover jump kick oneup pause pipe powerupappear powerupcollect shrink stomp timertick".split()
musics="castle gamewon overworld superstar underground underwater".split()
def synth(path,notes,volume=.18,duration=.17,repeat=1):
    path.parent.mkdir(parents=True,exist_ok=True)
    rate=22050;pcm=bytearray()
    for _ in range(repeat):
        for note in notes:
            f=440*2**((note-69)/12) if note else 0
            for n in range(int(rate*duration)):
                t=n/rate
                shape=math.sin(2*math.pi*f*t)+.18*math.sin(4*math.pi*f*t)
                fade=min(1,t*65)*min(1,(duration-t)*12)
                pcm.extend(struct.pack("<h",int(max(-1,min(1,shape*volume*fade))*32000)))
    with wave.open(str(path),"wb") as file:
        file.setnchannels(1);file.setsampwidth(2);file.setframerate(rate)
        file.writeframes(pcm)
for i,name in enumerate(effects):
    synth(ROOT/"sounds/effects"/(name+".wav"),[51+i%17,60+i%12,67+i%8],.15,.05)
for i,name in enumerate(musics):
    base=[52,57,60,65,55,59][i]
    notes=[base+n for n in [0,4,7,12,7,4,2,0,0,5,9,12,9,5,4,2]]
    synth(ROOT/"sounds/music"/(name+".wav"),notes,.075,.21,6)
# All levels below are fresh layouts; they do not reproduce Nintendo level geometry.
for world in range(1,9):
    for stage in range(1,5):
        rng=random.Random(2000+world*100+stage)
        width,height=128,15
        layers={k:[[-1 for x in range(width)]for y in range(height)] for k in (
            "Above_Foreground","Background","Collectibles","Enemies","Foreground","Underground")}
        fg=layers["Foreground"]
        for x in range(width):
            if x<15 or x>118 or x%24 not in (15,16):
                fg[13][x]=2+(world%6)*48
                fg[14][x]=3+(world%6)*48
        for x in range(20,width-17,18):
            elevation=8+(x//18+stage)%3
            for k in range(3+(world+stage)%2):
                fg[elevation][x+k]=2
            for k in range(2):
                layers["Collectibles"][elevation-1][x+k]=144+k
        for x in range(21,width-15,14):
            if fg[13][x]!=-1:layers["Enemies"][12][x]=0
        # End gate, tall original starlight mast and arrival wall for scene completion.
        for y in range(7,13):fg[y][111]=101
        fg[7][112]=152
        for y in range(8,13):fg[y][120]=2
        path=ROOT/"data"/f"World{world}-{stage}"
        path.mkdir(parents=True,exist_ok=True)
        for name,arr in layers.items():
            (path/f"World{world}-{stage}_{name}.csv").write_text(
                "\n".join(",".join(map(str,row))for row in arr)+"\n")
        nxt=(world,stage+1) if stage<4 else ((world+1,1) if world<8 else (0,0))
        props=f"""PLAYER_START = (2, 12)
CAMERA_START = (0, 0)
CAMERA_MAX = 105
LEVEL_TYPE = OVERWORLD
NEXT_LEVEL = ({nxt[0]}, {nxt[1]})
BACKGROUND_COLOR = BLUE
"""
        (path/f"World{world}-{stage}.levelproperties").write_text(props)
# A separately authored title-screen terrain. The Gold872 menu loads this
# file before its first render; without it the entire backdrop is absent.
menu=ROOT/"data"/"MenuBackground"
menu.mkdir(parents=True,exist_ok=True)
menu_rows=[[-1]*25 for _ in range(15)]
for x in range(25):
    menu_rows[13][x]=2+48
    menu_rows[14][x]=3+48
    if x in (2,5,20,23):
        menu_rows[11][x]=48+5
    if x in (3,21):
        menu_rows[10][x]=144
for x in range(25):
    if (x*7)%11<4:
        menu_rows[3][x]=193
(menu/"MenuBackground_Background.csv").write_text(
    "\n".join(",".join(map(str,row)) for row in menu_rows)+"\n")

# The original command-driven scene parser may also look up World0-0 on win.
(ROOT/"data"/"World0-0.levelproperties").write_text("LEVEL_TYPE = NONE\nNEXT_LEVEL = (0, 0)\n")
# Licensing: Liberation Sans is under the SIL Open Font License via distro pack.
font_options=[
    (Path("/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf"),Path("/usr/share/doc/fonts-liberation2/copyright")),
    (Path("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"),Path("/usr/share/doc/fonts-dejavu-core/copyright"))
]
for font,licensepath in font_options:
    if not font.is_file():continue
    dest=ROOT/"fonts"/"press-start-2p.ttf"
    dest.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(font,dest)
    if licensepath.is_file():shutil.copyfile(licensepath,ROOT/"fonts"/"FONT-LICENSE.txt")
    break
else:raise RuntimeError("No freely licensed system TTF font available")
print("Generated 3 original spritesheets, 4 UI images, 27 original sound files and 32 original level sets")
