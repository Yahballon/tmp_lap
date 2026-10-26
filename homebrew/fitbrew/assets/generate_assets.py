#!/usr/bin/env python3
"""FITBREW v0.1.3 offline photo staging.

Uses original AI-generated workout JPGs from the repository, never the prior
Unsplash placeholders. Prepares package-ready 512x512 JPEG + RGBA8, plus icon.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageOps, ImageEnhance, ImageFilter
from io import BytesIO
import hashlib
import shutil
import sys
import time
import urllib.request

SRC = Path(__file__).resolve().parent
TARGET = Path(sys.argv[1])
(TARGET / 'assets').mkdir(parents=True, exist_ok=True)
(TARGET / 'sce_sys').mkdir(parents=True, exist_ok=True)
PHOTO_COUNT=12

def source_photo(index: int):
    file=SRC/f"pose_{index}.jpg"
    if not file.is_file():
        raise RuntimeError(f"FITBREW AI source missing: {file}. Run import_ai_photos.py")
    with Image.open(file) as im:
        if im.width<256 or im.height<256:
            raise RuntimeError(f"FITBREW invalid AI photo dimensions: {file}")
        return im.convert("RGB")

def grade_photo(image):
    image = ImageOps.fit(image, (512,512), method=Image.Resampling.LANCZOS,
                         centering=(.50,.43))
    image = ImageEnhance.Color(image).enhance(.74)
    image = ImageEnhance.Contrast(image).enhance(1.17)
    image = ImageEnhance.Brightness(image).enhance(.84)
    # Fine dark vignette on the sides makes oversized TV cards coherent.
    matte = Image.new('RGBA',(512,512),(0,0,0,0))
    pix=matte.load()
    for y in range(512):
        for x in range(512):
            edge = min(x,511-x)/256
            darkness = int(48 * max(0.0,1.0-edge) ** 1.3)
            pix[x,y]=(4,9,16,darkness)
    return Image.alpha_composite(image.convert('RGBA'),matte).convert('RGB')

for i in range(PHOTO_COUNT):
    im=grade_photo(source_photo(i))
    jpg=TARGET / 'assets' / f'pose_{i}.jpg'
    raw=TARGET / 'assets' / f'pose_{i}.rgba'
    im.save(jpg,format='JPEG',quality=89,optimize=True,subsampling=0)
    rgba=im.convert('RGBA').tobytes()
    assert len(rgba)==512*512*4
    raw.write_bytes(rgba)
    print(f'PHOTO_READY pose_{i}: JPEG={jpg.stat().st_size} RGBA={raw.stat().st_size}')

# A real-photo launcher icon with the native branding.
icon=grade_photo(source_photo(0)).convert('RGBA')
veil=Image.new('RGBA',(512,512),(0,0,0,0))
mask=ImageDraw.Draw(veil)
mask.rounded_rectangle((13,340,499,502),radius=18,fill=(5,10,18,234))
icon=Image.alpha_composite(icon,veil)
d=ImageDraw.Draw(icon)
for font_path in ['/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf',
                  '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf']:
    if Path(font_path).is_file():
        font=ImageFont.truetype(font_path,78);break
else: font=ImageFont.load_default()
d.text((34,371),'FIT',font=font,fill='#ffffff')
d.text((176,371),'BREW',font=font,fill='#d9ff00')
d.line((36,465,477,465),fill='#d9ff00',width=7)
icon.convert('RGB').save(TARGET/'sce_sys'/'icon0.png',optimize=True)
print(f'PASS FITBREW AI photo assets: {PHOTO_COUNT} original AI workout images and launcher icon')
