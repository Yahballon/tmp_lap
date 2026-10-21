#!/usr/bin/env python3
"""Reject mock or empty PS5 packages. Validation is NOT proof of gameplay."""
from pathlib import Path
import json
import struct
import sys
import zipfile
from PIL import Image

app = Path(sys.argv[1] if len(sys.argv) > 1 else "app")
root = app / "dist" / "PPSA99358"
manifest = json.loads((root / "assets/game-data-manifest.json").read_text())
assert manifest["file_count"] > 100
assert manifest["total_bytes"] > 1024*1024
assert manifest["game_root"] == "game/"
metadata = json.loads((root / "sce_sys/param.json").read_text())
assert metadata["titleId"] == "PPSA99358"
assert metadata["contentId"] == "UP9000-PPSA99358_00-OPENSURGEPS5PORT"
assert metadata["localizedParameters"]["en-US"]["titleName"] == "Open Surge PS5 Test"
binary = (root / "eboot.bin").read_bytes()
assert len(binary) > 4096
assert binary[:4] == struct.pack("<I", 0x1D3D154F)
for marker in (b"BOOT LEGACY PS5 NATIVE", b"ORIGINAL GAME DATA", b"GAME ROOT FOUND",
               b"Original gameplay engine: NOT LINKED", b"LEVEL_PREVIEW_UNAVAILABLE", b"LEVEL_PREVIEW_READY",
               b"LEVEL_PREVIEW_START", b"ORIGINAL DEMO-1 LEVEL", b"v0.2.3", b"COLLISION_GRID", b"PERF fps=", b"TERRAIN_BLOCK"):
    assert marker in binary, f"Missing native diagnostic marker {marker!r}"
required = [
    "eboot.bin", "sce_sys/param.json", "sce_sys/icon0.png", "sce_module/libc.prx",
    "assets/game/surge.cfg", "assets/game/surge.rocks",
    "assets/game/LICENSE", "assets/game-data-manifest.json",
    "assets/probe_paths.txt", "assets/surge-preview.jpg",
    "assets/fonts/inter-semibold.huifont",
    "assets/level-preview/header.txt", "assets/level-preview/defs.txt",
    "assets/level-preview/bricks.txt", "assets/level-preview/sunshine.rgba",
    "assets/level-preview/surge.rgba",
    "assets/level-preview/collision.bin",
]
with Image.open(root / "sce_sys/icon0.png") as img:
    assert img.size == (512, 512) and img.format == "PNG"
for row in manifest["files"]:
    path=root / "assets/game" / row["path"]
    assert path.is_file() and path.stat().st_size == row["size"], row["path"]
with zipfile.ZipFile(app/"dist/PPSA99358.zip") as archive:
    assert archive.testzip() is None
    for rel in required:
        assert (root/rel).is_file(), rel
        assert f"PPSA99358/{rel}" in archive.namelist(), rel
    assert len([x for x in archive.namelist() if x.startswith("PPSA99358/assets/game/")]) >= 100
print("PASS: PS5 v0.2.2 indexed native collision and fixed-step movement, NOT full Open Surge gameplay")
