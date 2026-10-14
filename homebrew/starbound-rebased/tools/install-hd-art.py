#!/usr/bin/env python3
"""Embed the six user-supplied HD PNGs into the PS5 game package source tree.

This MUST run after procedural generators and reference recoloring.
The graphics are complete assets, not an optional USB-based replacement.
"""
from pathlib import Path
from PIL import Image
import hashlib
import shutil

app=Path(__file__).resolve().parents[1]
source=app/"hd-assets"
dest=app/"res"/"sprites"
sizes={
    "characters/PlayerSpriteSheet.png":(806,532),
    "characters/EnemySpriteSheet.png":(1120,480),
    "blocks/BlockTileSheet.png":(1634,750),
    "icons/logo.png":(704,384),
    "icons/windowicon.png":(64,64),
    "backgrounds/alien_sky.png":(800,480),
}
for relative, size in sizes.items():
    p=source/relative
    assert p.is_file(), f"Missing supplied HD artwork: {p}"
    with Image.open(p) as im:
        assert im.format=="PNG" and im.size==size, (p, im.size)
        im.verify()
    target=dest/relative
    target.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(p,target)
    digest=lambda file: hashlib.sha256(file.read_bytes()).hexdigest()
    assert digest(p)==digest(target), relative
    print("HD ART EMBEDDED:",relative,size)
assert len(list(source.rglob("*.png")))==len(sizes), "Unexpected extra HD images"
