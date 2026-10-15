#!/usr/bin/env python3
from pathlib import Path
import sys
root=Path(sys.argv[1])
out=root/'ABooks'/'LongAudio.M4B'
out.parent.mkdir(parents=True,exist_ok=True)
# Authentic RAR4 identification bytes followed by deterministic test payload.
with out.open('wb') as f:
    f.write(bytes.fromhex('00000018667479706d703432000000006d70343269736f6d'))
    chunk=bytes(range(256))*256
    for _ in range(768):
        f.write(chunk)
assert out.stat().st_size>40*1024*1024
print('Generated',out.stat().st_size,'bytes')
