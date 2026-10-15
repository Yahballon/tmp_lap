#!/usr/bin/env python3
"""Generate copyright-free CBZ pages and an unsafe/empty archive."""
from pathlib import Path
from io import BytesIO
from PIL import Image,ImageDraw
from zipfile import ZipFile,ZIP_DEFLATED
import sys
folder=Path(sys.argv[1]);folder.mkdir(parents=True,exist_ok=True)
def page(n):
    image=Image.new("RGB",(500,750),(35+n*7,60,105))
    dr=ImageDraw.Draw(image)
    dr.rectangle((40,35,460,715),outline=(240,180,45),width=14)
    dr.text((80,95),f"ORIGINAL TEST PAGE {n}",fill=(255,255,255))
    stream=BytesIO();image.save(stream,format="JPEG",quality=85);return stream.getvalue()
with ZipFile(folder/"sample.cbz","w",ZIP_DEFLATED) as zip:
    zip.writestr("README.txt","CBZ comic fixture")
    for n in (10,1,2):
        zip.writestr(f"Comic/page{n}.jpg",page(n))
    zip.writestr("Comic\\page5.jpg",page(5))
    zip.writestr("../bad.jpg",page(19))
with ZipFile(folder/"empty.cbz","w",ZIP_DEFLATED) as zip:
    zip.writestr("README.txt","No images")
