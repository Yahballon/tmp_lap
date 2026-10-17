#!/usr/bin/env python3
import json
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
app = Path(sys.argv[1])
meta = json.loads(Path("homebrew/psjukebox/app.json").read_text())
param = app / "sce_sys/param.json"
data = json.loads(param.read_text())
data.update(titleId=meta["titleId"], conceptId=meta["titleId"][4:],
            contentId=meta["contentId"], contentVersion=meta["contentVersion"])
data["localizedParameters"]["en-US"]["titleName"] = meta["name"]
param.write_text(json.dumps(data, indent=2)+"\n")
# Original generated icon, without any third-party artist assets.
img = Image.new("RGB", (512,512), (12,15,31))
d = ImageDraw.Draw(img)
for i, h in enumerate((98,158,232,312,250,193,272,140,106)):
 x = 74+i*41
 d.rounded_rectangle((x,392-h,x+24,392), radius=10, fill=(255,189+min(i*3,50),45))
d.ellipse((187,98,325,235),outline=(248,199,54),width=13)
d.arc((208,123,302,242),40,305,fill=(248,199,54),width=13)
d.rectangle((270,206,288,268),fill=(248,199,54))
try:
 font=ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",39)
except OSError:
 font=ImageFont.load_default()
d.text((256,444),"PSJUKEBOX",font=font,fill=(255,255,255),anchor="mm")
img.save(app/"sce_sys/icon0.png")
img.save(app/"assets/icon0.png")
print("Prepared PSJukebox identity and original icon")
