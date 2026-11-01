#!/usr/bin/env python3
"""Verify signed PS5 title and produce TESTBUILDS-compatible PPSA ZIP."""
import hashlib
import json
import pathlib
import struct
import sys
import zipfile

TITLE = "PPSA99053"

def package(directory: pathlib.Path):
    app = directory / TITLE
    exe = app / "eboot.bin"
    assert exe.is_file() and exe.stat().st_size > 100000, "Missing PS5 eboot.bin"
    assert exe.open("rb").read(4) == struct.pack("<I", 0x1D3D154F), "Not a PS5 signed SELF"
    metadata = json.loads((app / "sce_sys/param.json").read_text())
    assert metadata["titleId"] == TITLE
    assert metadata["contentId"] == "UP9000-PPSA99053_00-SCUMMVMPS5000001"
    for mandatory in ("sce_sys/icon0.png", "sce_module/libc.prx", "cores/scummvm_libretro.so", "info/scummvm_libretro.info"):
        assert (app / mandatory).is_file(), mandatory
    assert not list((app / "cores").glob("*_libretro.so")) or [p.name for p in (app / "cores").glob("*_libretro.so")] == ["scummvm_libretro.so"]
    out = directory / (TITLE + ".zip")
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for file in sorted(app.rglob("*")):
            if file.is_file():
                z.write(file, file.relative_to(directory))
    with zipfile.ZipFile(out) as z:
        assert z.testzip() is None
        assert f"{TITLE}/eboot.bin" in z.namelist()
    digest = hashlib.sha256(out.read_bytes()).hexdigest()
    (directory / "SHA256SUMS").write_text(f"{digest}  {TITLE}.zip\n")
    print(f"Validated {out} ({out.stat().st_size} bytes); SHA-256: {digest}")
    return out

if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("Usage: package.py <upstream-dist-dir>")
    package(pathlib.Path(sys.argv[1]))
