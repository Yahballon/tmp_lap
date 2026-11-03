#!/usr/bin/env python3
"""Validate and package a real signed AndroidFlow PS5 title; no source-only fake ZIP."""
import hashlib
import json
from pathlib import Path
from PIL import Image
import struct
import sys
import zipfile

TITLE = "PPSA99920"
CONTENT = "UP9000-PPSA99920_00-ANDROIDFLOW00001"

def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: package_native.py <dist/PPSA99920> <dist/PPSA99920.zip>")
    source, output = Path(sys.argv[1]), Path(sys.argv[2])
    if source.name != TITLE or output.name != TITLE + ".zip":
        raise ValueError("Unexpected title ID or ZIP name")
    required = ("eboot.bin", "sce_sys/param.json", "sce_sys/icon0.png",
                "sce_module/libc.prx", "androidflow.flag",
                "assets/fonts/Outfit-Regular.ttf", "assets/fonts/Outfit-Bold.ttf",
                "lib/libtest_arm64.so")
    for rel in required:
        path = source / rel
        if not path.is_file() or path.stat().st_size == 0:
            raise ValueError("Incomplete native PS5 title: " + rel)
    if (source/"eboot.bin").stat().st_size <= 1000000:
        raise ValueError("Native executable unexpectedly small")
    if (source/"eboot.bin").open("rb").read(4) != struct.pack("<I", 0x1D3D154F):
        raise ValueError("Missing signed SELF magic")
    meta = json.loads((source/"sce_sys/param.json").read_text())
    if meta["titleId"] != TITLE or meta["contentId"] != CONTENT:
        raise ValueError("Wrong title/content ID")
    img = Image.open(source/"sce_sys/icon0.png")
    if img.format != "PNG" or img.size != (512,512):
        raise ValueError("Invalid icon0.png")
    if "AndroidFlow" not in (source/"androidflow.flag").read_text():
        raise ValueError("Launcher flag invalid")
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for path in sorted(source.rglob("*")):
            if path.is_file():
                zi = zipfile.ZipInfo(TITLE+"/"+path.relative_to(source).as_posix())
                zi.compress_type = zipfile.ZIP_DEFLATED
                zi.external_attr = 0o100777 << 16
                z.writestr(zi, path.read_bytes())
    with zipfile.ZipFile(output) as z:
        assert z.testzip() is None
        assert all(TITLE+"/"+rel in z.namelist() for rel in required)
    digest = hashlib.sha256(output.read_bytes()).hexdigest()
    (output.parent/"SHA256SUMS").write_text(f"{digest}  {output.name}\n")
    print(f"PASS signed AndroidFlow native ZIP: {output}, sha256 {digest}")

if __name__ == "__main__":
    main()
