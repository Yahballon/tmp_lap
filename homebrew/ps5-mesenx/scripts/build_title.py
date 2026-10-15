#!/usr/bin/env python3
"""Build an actual signed PS5 MesenCE title using the PS5SNESX native linker.

The pipeline must stop on any missing library or SELF signing error. No mocks.
"""
from pathlib import Path
import json,os,shutil,subprocess,zipfile,struct,hashlib
ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"build/port-src"
FORK=ROOT/"build/MesenCE-Libretro"
CORE=ROOT/"build/ps5-core/libmesen_ce_nes.ps5.a"
DEPS=ROOT/"build/deps"
TITLE="PPSA99896"
APP="PS5MesenX"
def replace_once(text,old,new):
    assert text.count(old)==1,(old,text.count(old))
    return text.replace(old,new,1)
def run(*cmd,env=None):
    print("+",*map(str,cmd),flush=True)
    subprocess.run([str(c) for c in cmd],check=True,env=env)
def prepare():
    run("python3",ROOT/"scripts/prepare_frontend.py")
    ns=SRC/"snes/src"
    # The first milestone loads .nes ROMs. Remove ZIP/zlib and optional
    # NFS/curl link dependencies until native SDK libraries are ported.
    games=ns/"games.cpp"
    txt=games.read_text()
    txt=replace_once(txt,"#include <zlib.h>","")
    txt=replace_once(txt,
       'return RomName(name)||(name.size()>4 && Lower(name.substr(name.size()-4))==".zip");',
       'return RomName(name);')
    a=txt.index("static uint16_t U16(")
    b=txt.index("bool ReadRom(",a)
    txt=txt[:a]+txt[b:]
    a=txt.index('    if(path.size()>4&&Lower(path.substr(path.size()-4))==".zip") {')
    b=txt.index("    // (AI-assisted) Validate",a)
    txt=txt[:a]+"    rom=std::move(bytes);\n"+txt[b:]
    assert "z_stream" not in txt and "crc32" not in txt
    games.write_text(txt)
    shelf=ns/"shelf_games.cpp"
    shelf.write_text(shelf.read_text().replace('s==".nes"||s==".zip"','s==".nes"'))
    builder=SRC/"snes/tools/build_shelf.py"
    txt=builder.read_text()
    txt=replace_once(txt,"core=nes/'third_party/bsnes-mercury';sdk=",
                            "core=Path(os.environ['MESEN_CORE_SRC']);sdk=")
    a=txt.index("sources=subprocess.check_output(")
    b=txt.index("cpp=[*base,",a)
    txt=txt[:a]+'''incs=['-I'+str(nes/'ps5-compat'),'-I'+str(nes/'include'),'-I'+str(core/'Libretro'),'-I'+str(core)]
base=['-O2','-march=znver2','-ffunction-sections','-fdata-sections','-fno-stack-protector','-fno-plt']
cflags=['-target','x86_64-sie-ps5','-isysroot',str(sdk),'-isystem',str(sdk/'target/include'),'-std=gnu99',*base,*incs]
objects=[]
core_archive=Path(os.environ['MESEN_PS5_CORE'])
assert core_archive.is_file() and core_archive.stat().st_size>100000
print('Linking real PS5 MesenCE core:',core_archive,flush=True)
'''+txt[b:]
    for item in ("'-DORBIS_NFS=1',","'-DORBIS_CURL=1',","'-DCURL_STATICLIB',"):
        txt=txt.replace(item,"")
    txt=replace_once(txt,"objects.append(qrcode)\n","objects.append(qrcode)\nobjects.append(core_archive)\n")
    a=txt.index("netlibs=[")
    b=txt.index("\n\nstubs=",a)
    txt=txt[:a]+"netlibs=[]"+txt[b:]
    txt=txt.replace("str(nfs/'lib/libnfs.a'),","")
    a=txt.index("subprocess.run(['python3',str(deps/'filesystem-helper/tools/build-lapy-helper.py')")
    b=txt.index("licenses=app/'licenses'",a)
    txt=txt[:a]+txt[b:]
    txt=txt.replace("PPSA99205",APP)
    txt=txt.replace("PS5SNES-v6-Repository-Covers-by-YAHBALLON.zip",TITLE+".zip")
    txt=txt.replace("PS5SNES v6 by YAHBALLON",APP+" v0.1.0")
    txt=txt.replace("'bsnes-mercury balanced'","'MesenCE-Libretro NES HD'")
    txt=txt.replace("'https://github.com/libretro/bsnes-mercury'",
                    "'https://github.com/FrederickCampbell/MesenCE-Libretro'")
    txt=txt.replace("(core/'LICENSE','bsnes-mercury-GPL.txt')",
                    "(core/'LICENSE','MesenCE-Libretro-GPL.txt')")
    builder.write_text(txt)
    # Original independent asset; do not reuse the branded SNES icon.
    from PIL import Image,ImageDraw,ImageFont
    icon=SRC/"snes/sce_sys/icon0.png"
    im=Image.new("RGB",(512,512),(11,18,35))
    d=ImageDraw.Draw(im)
    d.rounded_rectangle((26,26,486,486),radius=80,fill=(27,43,67),outline=(66,187,219),width=7)
    f=ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",66)
    small=ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",35)
    d.text((82,158),"MESEN",fill=(242,247,255),font=f)
    d.text((204,250),"X",fill=(66,187,219),font=f)
    d.text((112,366),"HD NES",fill=(179,202,217),font=small)
    im.save(icon)
    assert json.loads((SRC/"snes/sce_sys/param.json").read_text())["titleId"]==TITLE
    return builder
def main():
    assert CORE.is_file(), "PS5 MesenCE core archive not compiled"
    builder=prepare()
    env=dict(os.environ)
    env.update(PS5SNES_DEPS=str(DEPS),MESEN_CORE_SRC=str(FORK),
        MESEN_PS5_CORE=str(CORE),
        PS5_PAYLOAD_SDK=str(DEPS/"PS5_Vulkan/.deps/native/ps5-payload-sdk"),
        LIBNFS_PREFIX=str(DEPS/"unused-libnfs"),
        LIBZ_PREFIX=str(DEPS/"unused-zlib"))
    run("python3",builder,env=env)
    built=SRC/"snes/build-shelf"/(TITLE+".zip")
    assert built.is_file(), "Signed PS5 ZIP not built"
    with zipfile.ZipFile(built) as z:
        assert z.testzip() is None
        data=z.read(APP+"/eboot.bin")
        assert len(data)>100000 and data[:4]==struct.pack("<I",0x1D3D154F)
        assert APP+"/sce_sys/param.json" in z.namelist()
        assert APP+"/sce_module/libc.prx" in z.namelist()
    out=ROOT/"dist"
    out.mkdir(exist_ok=True)
    shutil.copy2(built,out/(TITLE+".zip"))
    print("PASS PS5 native signed eboot.bin size=",len(data),"sha256",hashlib.sha256(data).hexdigest(),flush=True)
if __name__=="__main__":
    main()
