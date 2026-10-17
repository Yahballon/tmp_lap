#!/usr/bin/env python3
"""Fetch the pinned upstream SQLite amalgamation for a standalone PS5 ELF build.

The WKAL upstream SDK does not ship sqlite3.h or libsqlite3, so this compiles
SQLite directly into the installer. Source is SQLite 3.46.1, sha3-pinned.
"""
from pathlib import Path
import hashlib
import io
import urllib.request
import zipfile
import sys

SQLITE_URL = "https://www.sqlite.org/2024/sqlite-amalgamation-3460100.zip"
SHA3_256 = "af6aae8d3eccc608857c63cf56efbadc70da48b5c719446b353ed88dded1e288"

def main(root: Path):
    dest = root / "vendor/sqlite"
    dest.mkdir(parents=True, exist_ok=True)
    c = dest / "sqlite3.c"
    h = dest / "sqlite3.h"
    if c.is_file() and h.is_file():
        print("Using already-staged pinned SQLite amalgamation")
        return
    req = urllib.request.Request(SQLITE_URL, headers={"User-Agent": "WKAL-GAME-v0.4.0-build"})
    with urllib.request.urlopen(req, timeout=90) as response:
        data = response.read(8 * 1024 * 1024)
    got = hashlib.sha3_256(data).hexdigest()
    if got != SHA3_256:
        raise ValueError(f"SQLite download SHA3 mismatch: got {got}, expected {SHA3_256}")
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        for name in ("sqlite3.c", "sqlite3.h"):
            full = f"sqlite-amalgamation-3460100/{name}"
            (dest / name).write_bytes(archive.read(full))
    print("Staged SQLite 3.46.1 C amalgamation (verified sha3-256)")

if __name__ == "__main__":
    main(Path(sys.argv[1]) if len(sys.argv) > 1 else Path("."))
