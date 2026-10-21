#!/usr/bin/env python3
"""Verify native PS5 cross-compile artifacts rather than trusting CI exit code."""
from __future__ import annotations
import os
import re
import subprocess
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[1]
work = Path(os.environ.get("OPENSURGE_WORK", str(PROJECT / ".work")))
prefix = work / "ps5-port-prefix"
for library in ("libsurgescript-static.a", "libphysfs.a"):
    path = prefix / "lib" / library
    if not path.is_file() or path.stat().st_size < 1024:
        raise SystemExit(f"Missing static library: {path}")
    members = subprocess.check_output(["llvm-ar-18", "t", str(path)], text=True).splitlines()
    if len(members) < 5:
        raise SystemExit(f"Unexpectedly empty native archive: {path}")
    print(f"{library}: {len(members)} object members")

for name in ("surgescript", "physfs"):
    cache = (work / f"build-{name}-ps5" / "CMakeCache.txt").read_text()
    if not re.search(r"CMAKE_C_COMPILER:FILEPATH=.*prospero-clang18", cache):
        raise SystemExit(f"{name}: compiler was not the PS5 wrapper")
    if not re.search(r"CMAKE_TOOLCHAIN_FILE:FILEPATH=.*ps5-static-toolchain.cmake", cache):
        raise SystemExit(f"{name}: toolchain configuration not verified")
    print(f"{name}: PS5-target wrapper verified in CMake cache")

for filename in ("surgescript.h", "physfs.h"):
    if not (prefix / "include" / filename).is_file():
        raise SystemExit(f"Missing native header: {filename}")
print("PASS: static PS5-target dependency ABI artifacts, NOT a game eboot.bin")
