#!/usr/bin/env python3
from pathlib import Path
import sys
root=Path(sys.argv[1])
out=root/'Books'/'LargeComic.CBR'
out.parent.mkdir(parents=True,exist_ok=True)
# Authentic RAR4 identification bytes followed by deterministic test payload.
with out.open('wb') as f:
    f.write(bytes.fromhex('526172211a0700'))
    chunk=bytes(range(256))*256
    for _ in range(256):
        f.write(chunk)
assert out.stat().st_size>16*1024*1024
print('Generated',out.stat().st_size,'bytes')
