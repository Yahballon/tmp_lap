#!/usr/bin/env python3
"""Generate tiny original, standards-shaped EPUB archives for host CI tests."""
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED, ZIP_STORED
import sys

dest = Path(sys.argv[1])
dest.mkdir(parents=True, exist_ok=True)

opf = """<?xml version="1.0" encoding="utf-8"?>
<package xmlns="http://www.idpf.org/2007/opf"
         xmlns:dc="http://purl.org/dc/elements/1.1/" version="3.0">
 <metadata><dc:title>Example Test Book</dc:title><dc:creator>Test Author</dc:creator></metadata>
 <manifest><item id="chapter1" href="chapters/chapter.xhtml" media-type="application/xhtml+xml"/></manifest>
 <spine><itemref idref="chapter1"/></spine>
</package>"""
chapter = """<html xmlns="http://www.w3.org/1999/xhtml"><body>
<h1>EPUB package resolution works</h1><p>Hello from the example EPUB chapter.</p>
</body></html>"""
containers = {
 "standard": """<?xml version="1.0"?>
<container version="1.0" xmlns="urn:oasis:names:tc:opendocument:xmlns:container">
  <rootfiles>
    <rootfile full-path="OPS/book.opf" media-type="application/oebps-package+xml"/>
  </rootfiles>
</container>""",
 "namespaced": """<?xml version='1.0'?>
<ocf:container xmlns:ocf='urn:oasis:names:tc:opendocument:xmlns:container'>
  <ocf:rootfiles><ocf:rootfile media-type='application/oebps-package+xml'
           full-path = 'OPS/book.opf'/></ocf:rootfiles>
</ocf:container>""",
 "with_comment": """<container><!-- <rootfile full-path='DECOY.opf' /> -->
<rootfiles><rootfile full-path="OPS/book.opf" /></rootfiles></container>""",
 "multi_root": """<container><rootfiles><rootfile media-type="invalid"/><rootfile full-path="OPS/book.opf"/></rootfiles></container>""",
 "traversal": """<container><rootfiles><rootfile full-path="../../invalid.opf"/></rootfiles></container>""",
 "missing_root": """<container><rootfiles></rootfiles></container>""",
}
for name, container in containers.items():
    target = dest / f"{name}.epub"
    with ZipFile(target, "w") as z:
        z.writestr("mimetype", "application/epub+zip", compress_type=ZIP_STORED)
        z.writestr("META-INF/container.xml", container, compress_type=ZIP_DEFLATED)
        z.writestr("OPS/book.opf", opf, compress_type=ZIP_DEFLATED)
        z.writestr("OPS/chapters/chapter.xhtml", chapter, compress_type=ZIP_DEFLATED)
    print("Fixture:", target)

# Two original illustration fixtures: EPUB3 properties="cover-image" and
# EPUB2 <meta name="cover">. Both contain a mixed text/illustration spine.
from PIL import Image, ImageDraw
from io import BytesIO
cover = Image.new("RGB", (300, 420), "#54321d")
ImageDraw.Draw(cover).rectangle((35, 35, 265, 385), outline="#d5a64c", width=12)
inline = Image.new("RGB", (140, 90), "#256080")
ImageDraw.Draw(inline).ellipse((35, 10, 105, 80), fill="#f0c060")
def encode(image, ext):
    out = BytesIO()
    image.save(out, format=ext)
    return out.getvalue()
cover_bytes = encode(cover, "JPEG")
inline_bytes = encode(inline, "PNG")
for version in (2, 3):
    if version == 3:
        metadata = ""
        cover_attrs = 'properties="cover-image"'
    else:
        metadata = '<meta name="cover" content="cover-id"/>'
        cover_attrs = ''
    image_opf = f"""<package xmlns:dc="http://purl.org/dc/elements/1.1/">
    <metadata><dc:title>Example Test Book</dc:title><dc:creator>Test Author</dc:creator>{metadata}</metadata>
    <manifest><item id="chapter" href="chapters/part.xhtml" media-type="application/xhtml+xml"/>
    <item id="cover-id" href="images/front.jpg" media-type="image/jpeg" {cover_attrs}/>
    <item id="figure" href="images/scene.png" media-type="image/png"/>
    </manifest><spine><itemref idref="chapter"/></spine></package>"""
    image_chapter = """<html><body><p>Hello from the example EPUB chapter.</p>
        <p>Before the illustration.</p><img src="../images/scene.png" alt="Scene inside book"/>
        <p>After the illustration.</p></body></html>"""
    with ZipFile(dest / f"with_images_epub{version}.epub", "w") as z:
        z.writestr("mimetype", "application/epub+zip", compress_type=ZIP_STORED)
        z.writestr("META-INF/container.xml", containers["standard"], compress_type=ZIP_DEFLATED)
        z.writestr("OPS/book.opf", image_opf, compress_type=ZIP_DEFLATED)
        z.writestr("OPS/chapters/part.xhtml", image_chapter, compress_type=ZIP_DEFLATED)
        z.writestr("OPS/images/front.jpg", cover_bytes, compress_type=ZIP_DEFLATED)
        z.writestr("OPS/images/scene.png", inline_bytes, compress_type=ZIP_DEFLATED)
