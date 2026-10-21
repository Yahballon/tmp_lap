#!/usr/bin/env python3
"""End-to-end assertions on the preview cache built from genuine Open Surge."""
from pathlib import Path
import json
import sys

app=Path(sys.argv[1] if len(sys.argv)>1 else "app")
root=app/"assets"/"level-preview"
header=(root/"header.txt").read_text().splitlines()
meta={row.split()[0]:tuple(map(int,row.split()[1:])) for row in header}
assert set(meta)=={"atlas","hero","spawn","instances"}
defs=[row.split() for row in (root/"defs.txt").read_text().splitlines()]
instances=[row.split() for row in (root/"bricks.txt").read_text().splitlines()]
assert len(defs)>=90
assert len(instances)==meta["instances"][0]
assert len(instances)>=1000
assert meta["spawn"]==(176,9792), "Pinned original demo spawn changed"
for filename,dimension in (("sunshine.rgba",meta["atlas"]),("surge.rgba",meta["hero"])):
    assert (root/filename).stat().st_size==dimension[0]*dimension[1]*4
    pixels=(root/filename).read_bytes()
    # Original magenta transparency must be zero alpha, not hot-pink blocks.
    assert not any(pixels[i:i+3]==b"\\xff\\x00\\xff" and pixels[i+3]!=0
                   for i in range(0,len(pixels),4)), filename
collision=(root/"collision.bin").read_bytes()
assert len(collision)==meta["atlas"][0]*meta["atlas"][1]
assert all(pixel in (0,1) for pixel in collision)
assert 1000<sum(collision)<len(collision)*.95
by_id={int(row[0]):row for row in defs}
assert len(by_id)==len(defs)
assert {row[5] for row in defs}>={"SOLID","CLOUD","PASSABLE"}
assert all(int(row[0]) in by_id for row in instances)
assert all(0<=int(row[1])<=int(row[1])+int(row[3])<=meta["atlas"][0]
           and 0<=int(row[2])<=int(row[2])+int(row[4])<=meta["atlas"][1]
           for row in defs)
manifest=json.loads((app/"assets"/"game-data-manifest.json").read_text())
assert manifest["file_count"]>100
print("PASS original demo terrain/sprites staged for PS5: "
      f"{len(instances)} bricks, {len(defs)} types, "
      f"{meta['atlas']} RGBA atlas, real player sprite; NOT SurgeScript gameplay")
