#!/usr/bin/env python3
"""Build native DevBridge assets from the ORIGINAL game's pinned data.
This is not a substitute for the original game engine or an Allegro port.
"""
from pathlib import Path
from PIL import Image, ImageDraw
import json
import shutil
import sys

source = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(".work/ps5-staging")
app = Path(sys.argv[2]) if len(sys.argv) > 2 else Path("app")
game = source / "game"
assert game.is_dir() and (game / "surge.cfg").is_file()
manifest = json.loads((source / "game-data-manifest.json").read_text())
assert manifest["file_count"] > 100 and manifest["total_bytes"] > 1024*1024
dest = app / "assets"
dest.mkdir(parents=True, exist_ok=True)
assert not (dest / "game").exists(), "Refusing stale assets"
shutil.copytree(game, dest / "game", symlinks=False)
shutil.copy2(source / "game-data-manifest.json", dest / "game-data-manifest.json")
categories = [
    ("CONFIG", "surge.cfg"), ("QUEST", "surge.rocks"),
]
for folder, label in (("levels", "LEVEL"), ("scripts", "SCRIPT"), ("sprites", "SPRITE"),
                      ("musics", "MUSIC"), ("images", "IMAGE"), ("fonts", "FONT")):
    matches = sorted(p for p in (game / folder).rglob("*") if p.is_file())
    if matches:
        categories.append((label, matches[0].relative_to(game).as_posix()))
assert len(categories) >= 6, "Not enough actual upstream game data found"
(dest / "probe_paths.txt").write_text(
    "".join(f"{label}\t{rel}\n" for label, rel in categories), encoding="utf-8")

with Image.open(game / "surge.png") as src:
    original = src.convert("RGBA")
    original.thumbnail((1700, 850), Image.Resampling.LANCZOS)
    panel = Image.new("RGB", (1920, 1080), "#082a2e")
    panel.paste(original, ((1920-original.width)//2, (1080-original.height)//2),
                original.getchannel("A"))
    panel.save(dest / "surge-preview.jpg", quality=88, optimize=True)
    # Use authentic Open Surge picture with original license, not a counterfeit logo.
    icon = Image.new("RGBA", (512, 512), "#0b2234")
    original.thumbnail((440, 440), Image.Resampling.LANCZOS)
    icon.alpha_composite(original, ((512-original.width)//2, (512-original.height)//2))
    target = app / "sce_sys" / "icon0.png"
    target.parent.mkdir(parents=True, exist_ok=True)
    icon.convert("RGB").save(target, optimize=True)

assert Image.open(app/"sce_sys/icon0.png").size == (512, 512)
print(f"Staged complete original game data: {manifest['file_count']} files, {manifest['total_bytes']} bytes")
print("Native diagnostic assets generated, NOT gameplay engine")
