#!/usr/bin/env python3
import json,sys,zipfile,struct
from pathlib import Path
from PIL import Image, ImageChops
meta=json.loads(Path("homebrew/psjukebox/app.json").read_text())
name=meta["titleId"]
p=Path(sys.argv[1])
assert p.is_file(),p
root=p.parent/name
binary=(root/"eboot.bin").read_bytes()
assert len(binary)>1024 and struct.unpack("<I",binary[:4])[0]==0x1D3D154F
param=json.loads((root/"sce_sys/param.json").read_text())
assert param["titleId"]==name
assert param["contentId"]==meta["contentId"]
with Image.open(root/"sce_sys/icon0.png") as icon:
 assert icon.size==(512,512)
 with Image.open("homebrew/psjukebox/assets/neon-icon.webp") as source:
  expected=source.convert("RGB").resize((512,512),Image.Resampling.LANCZOS)
  assert ImageChops.difference(icon.convert("RGB"),expected).getbbox() is None, "PS5 menu icon is not the new neon jukebox artwork"
with Image.open(root/"assets/icon0.png") as app_icon:
 assert app_icon.size==(512,512)
 with Image.open(root/"sce_sys/icon0.png") as menu_icon:
  assert ImageChops.difference(app_icon.convert("RGB"),menu_icon.convert("RGB")).getbbox() is None, "In-app icon differs from PS5 tile icon"
helper=root/"jbxscan.elf"
assert helper.is_file() and helper.stat().st_size>1024
assert helper.read_bytes()[:4]==bytes([127,69,76,70])
with zipfile.ZipFile(p) as z:
 assert z.testzip() is None
 files=set(z.namelist())
 for rel in ("eboot.bin","sce_sys/param.json","sce_sys/icon0.png","assets/fonts/inter-semibold.huifont","assets/fonts/international.huifont","assets/fonts/DejaVu-LICENSE.txt","sce_module/libc.prx","jbxscan.elf"):
  assert name+"/"+rel in files,rel
# Verify the actual shipped bytes, not just the workspace font.
from tempfile import NamedTemporaryFile
import subprocess
with NamedTemporaryFile(suffix=".huifont") as temp:
 temp.write((root/"assets/fonts/international.huifont").read_bytes());temp.flush()
 subprocess.run([sys.executable,"homebrew/psjukebox/tools/verify-international-font.py",temp.name],check=True)
print("Verified installable",p)
