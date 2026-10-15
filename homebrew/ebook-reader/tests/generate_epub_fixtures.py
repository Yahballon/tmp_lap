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
