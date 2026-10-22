#!/usr/bin/env python3
"""Package the *real* signed upstream zplayer FSELF as an isolated PS5 boot test.

Never substitute the working PPSA99127 Quest Browser, fabricate a game
engine, or mark this prerelease as playable. Real console boot unverified.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import re
import shutil
import struct
import zipfile
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

parser = argparse.ArgumentParser()
parser.add_argument("--fself", type=Path, required=True)
parser.add_argument("--runtime-prx", type=Path, required=True)
parser.add_argument("--template-param", type=Path, required=True)
parser.add_argument("--upstream", type=Path, required=True)
parser.add_argument("--icon", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args()

TITLE = "PPSA99128"
CONTENT = "UP9000-PPSA99128_00-ZQUESTBOOTTEST00"
assert len("ZQUESTBOOTTEST00") == 16
assert args.fself.stat().st_size > 1024 * 1024, "Missing full zplayer FSELF"
assert args.fself.read_bytes()[:4] == struct.pack("<I", 0x1D3D154F), "Not a PS5 FSELF"
assert args.runtime_prx.stat().st_size > 100_000
assert args.runtime_prx.read_bytes()[:4] in (struct.pack("<I", 0x1D3D154F), struct.pack("<I", 0xEEF51454)), "Expected signed PS5 runtime format"
assert (args.upstream / "LICENSE").is_file()
assert (args.upstream / "resources" / "allegro5.cfg").is_file()
assert (args.upstream / "src" / "zc" / "zelda.cpp").is_file()

folder = args.output / TITLE
if folder.exists():
    shutil.rmtree(folder)
(folder / "sce_sys").mkdir(parents=True)
(folder / "sce_module").mkdir()
shutil.copy2(args.fself, folder / "eboot.bin")
shutil.copy2(args.runtime_prx, folder / "sce_module" / "libc.prx")
shutil.copy2(args.upstream / "LICENSE", folder / "LICENSE-ZQUESTCLASSIC.txt")
if (args.upstream / "resources" / "README.txt").is_file():
    shutil.copy2(args.upstream / "resources" / "README.txt", folder / "ZQUEST-RESOURCES-LICENSE.txt")

data = json.loads(args.template_param.read_text())
data.update(titleId=TITLE, conceptId=TITLE[4:], contentId=CONTENT,
            contentVersion="01.000.002")
data["localizedParameters"]["en-US"]["titleName"] = "ZQuest Engine BOOT TEST"
data["localizedParameters"]["en-US"]["titleDescription"] = (
    "Experimental zplayer native PS5 boot diagnostic. NOT PLAYABLE."
)
(folder / "sce_sys" / "param.json").write_text(json.dumps(data, indent=2) + "\n")

with Image.open(args.icon) as original:
    im = original.convert("RGB").resize((512, 512))
draw = ImageDraw.Draw(im)
draw.rounded_rectangle((28, 373, 484, 472), 12, fill="#2a1013",
                       outline="#ff9871", width=4)
font_file = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
font = ImageFont.truetype(font_file, 28) if Path(font_file).is_file() else ImageFont.load_default()
draw.text((256, 407), "ENGINE BOOT", anchor="mm", font=font, fill="#fff0c1")
draw.text((256, 447), "EXPERIMENTAL", anchor="mm", font=font, fill="#ffaf8f")
im.save(folder / "sce_sys" / "icon0.png", optimize=True)

# Bundle upstream redistributable support resources, not original quest files
# or optional NSF sample music. No licensed game/ROM assets are injected.
resource_src = args.upstream / "resources"
resource_dest = folder / "resources"
shutil.copytree(resource_src, resource_dest, ignore=shutil.ignore_patterns(
    "*.qst", "*.nsf", ".DS_Store", "*.exe", "*.dll", "*.dylib", "*.so"))
assert (resource_dest / "allegro5.cfg").is_file()
assert not list(resource_dest.rglob("*.qst"))
assert not list(resource_dest.rglob("*.nsf"))
(folder / "BOOT_TEST_README.txt").write_text(
    "ZQuestClassic original zplayer PS5 engine BOOT TEST v0.0.2\n"
    "HARDWARE STARTUP/QUEST PLAYBACK NOT VERIFIED. May crash/hang.\n"
    "Separate PPSA99128. Keep working Quest Browser PPSA99127 v0.1.5.\n"
    "Use PS5 jailbreak and launch this app; report first on-screen behavior.\n"
    "Startup log (after libc init): /data/ZQuestEngineBoot/logs/boot.log\n"
    "Crash signal log: /data/ZQuestEngineBoot/logs/crash.log\n"
    "Fallback: /data/ZQuestClassic/logs/engine-boot.log and engine-crash.log\n"
    "Send the logs after any crash; missing files may mean a pre-libc fault.\n"
    "Original .qst quest selection is not yet attached to the engine boot.\n"
    "Original upstream GPLv3 source: https://github.com/ZQuestClassic/ZQuestClassic\n"
)
zip_path = args.output / f"{TITLE}.zip"
with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
    for item in sorted(folder.rglob("*")):
        if item.is_file():
            archive.write(item, item.relative_to(args.output).as_posix())
with zipfile.ZipFile(zip_path) as check:
    assert check.testzip() is None
    names = check.namelist()
    for suffix in ("eboot.bin", "sce_sys/param.json", "sce_sys/icon0.png",
                   "sce_module/libc.prx", "resources/allegro5.cfg",
                   "BOOT_TEST_README.txt", "LICENSE-ZQUESTCLASSIC.txt"):
        assert f"{TITLE}/{suffix}" in names, suffix
    assert all(name.startswith(TITLE + "/") for name in names)
metadata = json.loads((folder / "sce_sys" / "param.json").read_text())
assert metadata["titleId"] == TITLE and metadata["contentId"] == CONTENT
assert re.fullmatch(r"PPSA\d{5}", TITLE)
digest = hashlib.sha256(zip_path.read_bytes()).hexdigest()
print(f"ZQUEST_REAL_ENGINE_BOOT_TEST_PACKAGE={zip_path}")
print(f"ZQUEST_BOOT_TEST_ZIP_BYTES={zip_path.stat().st_size}")
print(f"ZQUEST_BOOT_TEST_SHA256={digest}")
print("ZQUEST_PLAYABLE=NO | PS5_BOOT_VERIFIED=NO | QUEST_LAUNCH=UNVERIFIED")
