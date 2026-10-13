#!/usr/bin/env python3
"""Guard the new PS5 product from accidentally shipping Mario or upstream media."""
from pathlib import Path
import sys
upstream=Path(sys.argv[1]) if len(sys.argv)>1 else Path("build/gold872-engine")
if not (upstream/"src/main.cpp").is_file() or not (upstream/"LICENSE").is_file():
    sys.exit("Missing pinned Gold872 MIT engine source and license")
for forbidden in ("res","bin","screenshots"):
    if (upstream/forbidden).exists():
        sys.exit("Refusing copyrighted upstream asset directory: "+forbidden)
print("Source-only upstream checkout verified; game asset replacement still required")
