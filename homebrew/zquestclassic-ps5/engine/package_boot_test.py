#!/usr/bin/env python3
"""Create a clearly NON-PLAYABLE PS5 zplayer engine boot-test install ZIP.

This runs only after the native converter has produced a genuine eboot.bin.
Uses a *different* title ID from the working ZQuest Classic quest browser,
so testers can install it without replacing v0.1.5. It does not pretend to
contain gameplay resources or to be hardware-verified.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import zipfile

p=argparse.ArgumentParser()
p.add_argument("--eboot",type=Path,required=True)
p.add_argument("--param-template",type=Path,required=True)
p.add_argument("--icon",type=Path,required=True)
p.add_argument("--out",type=Path,required=True)
a=p.parse_args()
assert a.eboot.is_file() and a.eboot.stat().st_size > 100_000, "No real converted FSELF"
assert a.icon.is_file() and a.icon.stat().st_size > 1000, "No app icon"
metadata=json.loads(a.param_template.read_text())
tid="PPSA99128"
cid="UP9000-PPSA99128_00-ZQUESTENGINEBOOT"
assert len(cid.split("_00-")[1])==16
assert metadata["localizedParameters"]["en-US"]["titleName"]
metadata.update(titleId=tid, conceptId=tid[4:],contentId=cid,
                contentVersion="01.000.001")
metadata["localizedParameters"]["en-US"]["titleName"]="ZQuest Engine BOOT TEST"
a.out.mkdir(parents=True,exist_ok=True)
root=a.out/tid
assert not root.exists(), "Refusing to overwrite an existing test app"
(root/"sce_sys").mkdir(parents=True)
shutil.copyfile(a.eboot,root/"eboot.bin")
shutil.copyfile(a.icon,root/"sce_sys"/"icon0.png")
(root/"sce_sys"/"param.json").write_text(json.dumps(metadata,indent=2)+"\n")
(root/"BOOT_TEST_ONLY.txt").write_text(
    "ZQuestClassic real zplayer experimental PS5 boot test\n"
    "Original ZQuestClassic game engine, no quest bundled.\n"
    "NOT PLAYABLE / NOT HARDWARE-VERIFIED. THIS MAY CRASH.\n"
    "Title ID PPSA99128 is intentionally different from working PPSA99127.\n"
    "Do not treat FSELF conversion as proof of functioning gameplay.\n")
zip_path=a.out/(tid+".zip")
with zipfile.ZipFile(zip_path,"w",compression=zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    for file in sorted(root.rglob("*")):
        if file.is_file():
            z.write(file,file.relative_to(a.out).as_posix())
with zipfile.ZipFile(zip_path) as z:
    assert z.testzip() is None
    assert f"{tid}/eboot.bin" in z.namelist()
    assert f"{tid}/sce_sys/param.json" in z.namelist()
print("Boot-test ZIP (not a verified playable app):",zip_path)
print("SHA256",hashlib.sha256(zip_path.read_bytes()).hexdigest())
