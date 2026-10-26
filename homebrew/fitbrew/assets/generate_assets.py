#!/usr/bin/env python3
"""FITBREW real-photo artwork pipeline for the PS5 offline installer.

This script downloads credited freely usable original fitness photographs DURING
GitHub Actions; the console never needs an internet connection. A missing photo
is a failed build, never a silent switch back to ugly geometric silhouettes.
Every image is prepared as a square JPEG and raw 512x512 RGBA8 GPU texture.
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
# All twelve source pages and author credits live in PHOTO_CREDITS.md.
PHOTO_IDS = [
    '1731341400836-baaa5535b8d5',
    '1727848562663-81fec613880c',
    '1765302741884-e846c7a178df',
    '1759476530203-b11cc8c87d43',
    '1631501700640-cfd8663b9bd7',
    '1485727749690-d091e8284ef3',
    '1789629455545-6418611bb5a2',
    '1734668491961-a3ea1168d37b',
    '1666979289446-6e9ebb257091',
    '1762709753339-7bd2fea1f346',
    '1759476597683-f51b04cf58d3',
    '1767611094260-1f5565e6d815'
]
PHOTO_COUNT = len(PHOTO_IDS)
CACHE = Path('/tmp/fitbrew-verified-photo-cache')
CACHE.mkdir(parents=True, exist_ok=True)

def source_photo(index: int):
    user_photo = SRC / f'pose_{index}.jpg'
    if user_photo.is_file():
        return Image.open(user_photo).convert('RGB')
    url = (f'https://images.unsplash.com/photo-{PHOTO_IDS[index]}'
           '?fit=crop&crop=entropy&w=1024&h=1024&fm=jpg&q=85')
    cache_file = CACHE / f'{PHOTO_IDS[index]}.jpg'
    if not cache_file.exists():
        err = None
        for attempt in range(3):
            try:
                req = urllib.request.Request(
                    url, headers={'User-Agent': 'FITBREW-PS5/0.1.2 (offline-packager)',
                                  'Accept': 'image/jpeg,image/*'})
                with urllib.request.urlopen(req, timeout=35) as response:
                    if response.status != 200:
                        raise RuntimeError(f'photo {index} HTTP {response.status}')
                    data = response.read(10 * 1024 * 1024 + 1)
                    if len(data) > 10 * 1024 * 1024 or len(data) < 12000:
                        raise RuntimeError(f'photo {index} bad source length {len(data)}')
                with Image.open(BytesIO(data)) as image:
                    image.verify()
                cache_file.write_bytes(data)
                break
            except Exception as ex:
                err = ex
                if attempt < 2: time.sleep(2 * (attempt + 1))
        if not cache_file.exists():
            raise RuntimeError(f'FITBREW photo_{index} download failed; '
                               f'refusing illustration fallback: {err}')
    print(f'PHOTO_SOURCE {index}: {len(cache_file.read_bytes())} bytes sha256='
          f'{hashlib.sha256(cache_file.read_bytes()).hexdigest()}', flush=True)
    return Image.open(cache_file).convert('RGB')

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
print(f'PASS FITBREW photo assets: {PHOTO_COUNT} credited photographs and launcher icon')
