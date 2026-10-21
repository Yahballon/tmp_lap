#!/usr/bin/env python3
"""Stage all Godot PS5 static libraries for a real DevBridge linker test."""
import argparse
import json
import shutil
from pathlib import Path

def stage(godot, app, entry):
    godot = Path(godot).resolve()
    app = Path(app).resolve()
    manifest = json.loads((godot/"godot-ps5-full-manifest.json").read_text())
    assert manifest["archive_count"] >= 50
    assert manifest["format"] == "godot-ps5-static-libs"
    assert manifest["target"] == "x86_64-sie-ps5"
    libraries = []
    for record in manifest["archives"]:
        rel = Path(record["path"])
        assert not rel.is_absolute() and ".." not in rel.parts
        source = godot/rel
        assert source.is_file() and source.stat().st_size == record["size_bytes"]
        dest = app/"godot-libs"/rel
        dest.parent.mkdir(parents=True,exist_ok=True)
        dest.symlink_to(source)
        libraries.append(dest.relative_to(app).as_posix())
    build = app/"tools/build.sh"
    source = build.read_text()
    prefix = 'link_inputs=("$build/obj/app_crt.o" "$build/obj/app_cpp_runtime.o" "${objects[@]}")'
    suffix = '    link_inputs+=("$root/$archive")'
    assert source.count(prefix) == 1 and source.count(suffix) == 1
    source = source.replace(prefix,prefix+'\nlink_inputs+=("--start-group")',1)
    source = source.replace(suffix+'\ndone',suffix+'\ndone\nlink_inputs+=("--end-group")',1)
    build.write_text(source)
    target=app/"src/godot-smoke"
    target.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(entry,target/"main.cpp")
    metadata=app/"sce_sys/param.json"
    p=json.loads(metadata.read_text())
    p.update(titleId="PPSA99843",contentId="UP9000-PPSA99843_00-GODOTPS5HEADLESS",conceptId="99843",contentVersion="01.000.001")
    p["localizedParameters"]["en-US"]["titleName"]="Godot PS5 Engine Smoke"
    metadata.write_text(json.dumps(p,indent=2)+'\n')
    (app/"GNUmakefile").write_text('APP_SOURCE_DIR := src/godot-smoke\nAPP_ASSETS := assets\nAPP_STATIC_ARCHIVES := '+' '.join(libraries)+'\ninclude Makefile\n')
    print("Staged",len(libraries),"Godot libraries for actual PS5 engine link")

if __name__=="__main__":
    a=argparse.ArgumentParser()
    a.add_argument("--godot",required=True)
    a.add_argument("--app",required=True)
    a.add_argument("--entry",required=True)
    x=a.parse_args()
    stage(x.godot,x.app,x.entry)
