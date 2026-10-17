#!/usr/bin/env python3
import json,sys,zipfile,struct
from pathlib import Path
from PIL import Image
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
with zipfile.ZipFile(p) as z:
 assert z.testzip() is None
 files=set(z.namelist())
 for rel in ("eboot.bin","sce_sys/param.json","sce_sys/icon0.png","assets/fonts/inter-semibold.huifont","sce_module/libc.prx"):
  assert name+"/"+rel in files,rel
print("Verified installable",p)
