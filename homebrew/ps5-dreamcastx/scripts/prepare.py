#!/usr/bin/env python3
"""Fetch a pinned, working PS5 Flycast core; adapt the native shell and shelf.

The big PSFlycast source tree remains unmodified upstream. No precompiled
emulator is used; GitHub Actions compiles this real PS5 C/C++ native port.
"""
from pathlib import Path
import json, re, shutil, subprocess

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/"build/flycast"
PIN="1cc6b2978d891f93cd2e36123ba4fd14118bd444"
TITLE="PPSA99765"
APP_FOLDER="PS5DreamcastX"
VERSION="0.1.1"

def cmd(*args):
    subprocess.run(args,check=True)

def prepare_checkout():
    SOURCE.parent.mkdir(parents=True,exist_ok=True)
    if not (SOURCE/".git").is_dir():
        if SOURCE.exists():
            raise RuntimeError("build/flycast exists and is not a Git checkout")
        cmd("git","clone","--recursive","https://github.com/Press5elect/PSFlycast.git",str(SOURCE))
    cmd("git","-C",str(SOURCE),"fetch","origin",PIN)
    cmd("git","-C",str(SOURCE),"checkout","--detach",PIN)
    cmd("git","-C",str(SOURCE),"submodule","update","--init","--recursive")

def adapt():
    p=SOURCE/"shell/ps5"
    ui=p/"bigpicture.cpp"
    if not ui.exists():
        raise RuntimeError("Expected PSFlycast native BigPicture UI is missing")
    for f in p.rglob("*"):
        if f.is_file() and f.suffix in (".cpp",".h",".sh",".py",".json",".cmake",".inc",".txt",".md"):
            t=f.read_text(encoding="utf-8")
            # Preserve copyright attribution to the original PSFlyCast authors.
            t=t.replace("PPSA99247",TITLE).replace("PSFlyCast","PS5DreamcastX")
            t=t.replace("Copyright 2026 the PS5DreamcastX contributors",
                "Copyright 2026 the PSFlyCast contributors")
            f.write_text(t,encoding="utf-8")

    t=ui.read_text()
    anchor="\t\tdrawShelves(dl, focused, selectDisk);"
    include='// ---- the grid view: every game\'s cover'
    if anchor in t:
        assert t.count(anchor)==1
        t=t.replace(anchor,"\t\tdrawDreamcastXShelf(dl, focused, selectDisk);")
    assert t.count("drawDreamcastXShelf(dl, focused, selectDisk);")==1
    if '#include "dreamcastx_shelf.inc"' not in t:
        assert t.count(include)==1, "Upstream library layout changed"
        t=t.replace(include,'#include "dreamcastx_shelf.inc"\n\n'+include)
    old='text(dl, bold(), 34, 112, 28, col::text, "PS5DreamcastX");'
    if old in t:
        t=t.replace(old,'text(dl, bold(), 28, 112, 18, col::text, "PS5DreamcastX");\n'
                    '\t\ttext(dl, regular(), 17, 114, 60, col::dim, "by YAHBALLON");',1)
    marker='text(dl, bold(), 30, W - 48 - cs.x, 48 - cs.y / 2, col::text, clock);'
    if marker in t and '"v0.1.1"' not in t:
        t=t.replace(marker,marker+'\n\t'
            'const ImVec2 vs = textSize(regular(), 17, "v0.1.1");\n\t'
            'text(dl, regular(), 17, W - 48 - vs.x, 76, col::dim, "v0.1.1");',1)
    ui.write_text(t)
    shutil.copy2(ROOT/"src/dreamcastx_shelf.inc",p/"dreamcastx_shelf.inc")

    # Rename the physical ShadowMount folder, not the mandatory PPSA titleId.
    main=p/"ps5_main.cpp"
    m=main.read_text()
    assert m.count('candidates.push_back("/data/homebrew/PPSA99765");') == 1
    m=m.replace('/data/homebrew/PPSA99765/', '/data/homebrew/PS5DreamcastX/')
    m=m.replace('candidates.push_back("/data/homebrew/PPSA99765");',
                'candidates.push_back("/data/homebrew/PS5DreamcastX");')
    start=m.index('void findUsbDirs()')
    end=m.index('// The folders in emu.cfg',start)
    m=m[:start]+'''#include "dreamcastx_usb.inc"

void findUsbDirs()
{
    ps5::usbDirs = discoverDreamcastXUsbDirs("/mnt/usb", 8);
    for (const std::string& dir : ps5::usbDirs)
        ps5::diag::mark("usb: DreamcastX game directory: %s", dir.c_str());
    ps5::diag::mark("usb: %d game folder(s)", (int)ps5::usbDirs.size());
}

'''+m[end:]
    main.write_text(m)
    shutil.copy2(ROOT/"src/dreamcastx_usb.inc",p/"dreamcastx_usb.inc")

    library=p/"ps5_library.cpp"
    l=library.read_text()
    l=l.replace('"/data/homebrew/PPSA99765/"',
                '"/data/homebrew/PS5DreamcastX/", "/data/homebrew/PPSA99765/"')
    library.write_text(l)

    # Fresh installs discover USB sources automatically when elfldr is available.
    # Existing frontend.cfg with usb=0 stays user-controlled (enable in Settings).
    frontend=p/"ps5_frontend.h"
    f=frontend.read_text()
    assert f.count("bool usb = false;") == 1
    frontend.write_text(f.replace("bool usb = false;", "bool usb = true;", 1))

    # Never offer another tmp_lap application's ZIP as an update.
    # The upstream download/update code uses a release list; constrain both
    # tag namespace and exact ZIP asset. No auto-update on this test build.
    updater=p/"ps5_update.cpp"
    u=updater.read_text()
    u=u.replace("Press5elect/PSFlycast/releases","Yahballon/tmp_lap/releases")
    # Self-update reads files from the packaged folder; the Title ID is NOT the installation path.
    u=u.replace('constexpr const char *TitleFolder = "PPSA99765/";',
                'constexpr const char *TitleFolder = "PS5DreamcastX/";')
    assert 'constexpr const char *TitleFolder = "PS5DreamcastX/";' in u
    tag='release.tag = entry["tag_name"].get<std::string>();'
    if tag not in u:
        raise RuntimeError("Upstream updater tag hook changed")
    u=u.replace(tag,tag+'''
    if (release.tag.rfind("ps5-dreamcastx-v", 0) != 0)
    {
        why = "not a PS5DreamcastX release";
        return false;
    }
    release.tag = release.tag.substr(sizeof("ps5-dreamcastx-") - 1);
''',1)
    wanted='const std::string wanted = "PS5DreamcastX-" + release.tag + ".zip";'
    if wanted not in u:
        raise RuntimeError("Expected renamed updater asset pattern missing")
    u=u.replace(wanted,'const std::string wanted = "PPSA99765.zip";',1)
    fallback='if (chosen == nullptr && endsWith(lower(name), ".zip") && lower(name).find("source") == std::string::npos)'
    if fallback not in u:
        raise RuntimeError("Upstream ZIP fallback changed; review safety")
    u=u.replace(fallback,'if (false && chosen == nullptr)',1)
    updater.write_text(u)

    ver=p/"ps5_build.h"
    v=ver.read_text()
    v,n=re.subn(r'#define PS5_BUILD_NUMBER\s+\d+','#define PS5_BUILD_NUMBER 2',v)
    assert n==1
    v,n=re.subn(r'#define PS5_VERSION\s+"[^"]+"','#define PS5_VERSION "0.1.1"',v)
    assert n==1
    ver.write_text(v)

    meta=p/"sce_sys/param.json"
    data=json.loads(meta.read_text())
    data["titleId"]=TITLE
    data["conceptId"]=TITLE[4:]
    data["contentId"]="UP9000-PPSA99765_00-PS5DREAMCASTX001"
    data["contentVersion"]="01.000.001"
    data["masterVersion"]="01.00"
    data["localizedParameters"]["en-US"]["titleName"]="PS5DreamcastX"
    meta.write_text(json.dumps(data,indent=2)+"\n")
    # Generated from an original vector design; original Flycast icon not copied.
    out=ROOT/"build/icon0.png"
    cmd("python3",str(ROOT/"assets/generate_icon.py"),str(out))
    shutil.copy2(out,p/"sce_sys/icon0.png")
    script=p/"build.sh"
    s=script.read_text()
    assert "title=PPSA99765" in s
    s=s.replace("title=PPSA99765", "title=PS5DreamcastX", 1)
    # Old PSFlyCast startup artwork/audio must not appear under our branding.
    s=re.sub(r'^cp -- "\$src/shell/ps5/sce_sys/pic0.dds".*?\n','',s,flags=re.M)
    s=re.sub(r'^cp -- "\$src/shell/ps5/sce_sys/snd0.at9".*?\n','',s,flags=re.M)
    script.write_text(s)
    for name in ("pic0.dds","pic1.dds","snd0.at9"):
        (p/"sce_sys"/name).unlink(missing_ok=True)
    print("Prepared Flycast PS5 core and GBAX-style native shelf",TITLE)

if __name__=="__main__":
    prepare_checkout()
    adapt()
