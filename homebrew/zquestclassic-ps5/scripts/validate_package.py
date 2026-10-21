#!/usr/bin/env python3
"""Validate actual PS5 ELF and install folder; do not equate with hardware test."""
import json
import struct
import sys
import zipfile
from pathlib import Path
from PIL import Image

base = Path(sys.argv[1] if len(sys.argv) > 1 else "app/dist")
title = "PPSA99127"
folder = base / title
metadata = json.loads((folder / "sce_sys" / "param.json").read_text())
assert metadata["titleId"] == title
assert metadata["contentId"] == "UP9000-PPSA99127_00-ZQUESTCLASSICPS5"
assert metadata["localizedParameters"]["en-US"]["titleName"] == "ZQuest Classic PS5"
binary = (folder / "eboot.bin").read_bytes()
assert len(binary) > 1000
assert binary[:4] == struct.pack("<I", 0x1D3D154F), "expected PS5 patched ELF"
assert b"ZQUEST CLASSIC" in binary and b"QUEST LIBRARY" in binary
assert b"v0.1.1" in binary, "Expected native scanner fix v0.1.1"
required = [
    "eboot.bin", "sce_sys/param.json", "sce_sys/icon0.png",
    "sce_module/libc.prx", "assets/fonts/inter-semibold.huifont"
]
with Image.open(folder / "sce_sys" / "icon0.png") as im:
    assert im.size == (512, 512) and im.format == "PNG"
archive = base / f"{title}.zip"
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    for entry in required:
        assert (folder / entry).is_file(), entry
        assert f"{title}/{entry}" in z.namelist(), entry
print("Native quest-browser package verified (ZQuest game engine NOT included)")
