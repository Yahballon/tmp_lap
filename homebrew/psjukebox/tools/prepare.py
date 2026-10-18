#!/usr/bin/env python3
import json
import sys
from pathlib import Path
from PIL import Image
app = Path(sys.argv[1])
meta = json.loads(Path("homebrew/psjukebox/app.json").read_text())
param = app / "sce_sys/param.json"
data = json.loads(param.read_text())
data.update(titleId=meta["titleId"], conceptId=meta["titleId"][4:],
            contentId=meta["contentId"], contentVersion=meta["contentVersion"])
data["localizedParameters"]["en-US"]["titleName"] = meta["name"]
param.write_text(json.dumps(data, indent=2)+"\n")
# The user-supplied neon jukebox image is the only app icon source.
# This explicitly replaces the previous procedural icon in both the PS5
# Home screen (sce_sys/icon0.png) and the app's own assets/icon0.png.
icon_source=Path("homebrew/psjukebox/assets/neon-icon.webp")
with Image.open(icon_source) as uploaded:
 img=uploaded.convert("RGB").resize((512,512),Image.Resampling.LANCZOS)
app.joinpath("sce_sys").mkdir(parents=True,exist_ok=True)
app.joinpath("assets").mkdir(parents=True,exist_ok=True)
img.save(app/"sce_sys/icon0.png",format="PNG",optimize=True)
img.save(app/"assets/icon0.png",format="PNG",optimize=True)
print("Prepared PSJukebox neon icon from",icon_source)
