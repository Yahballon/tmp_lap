#!/usr/bin/env python3
"""Fail-fast integration verification; no PS5 hardware claims."""
from pathlib import Path
import json,sys
root=Path(__file__).resolve().parents[1]
src=root/"build/flycast/shell/ps5"
assert src.is_dir(), "Flycast source has not been prepared"
ui=(src/"bigpicture.cpp").read_text()
assert ui.count('drawDreamcastXShelf(dl, focused, selectDisk);')==1
assert ui.count('#include "dreamcastx_shelf.inc"')==1
assert 'void drawDreamcastXShelf(' in (src/"dreamcastx_shelf.inc").read_text()
meta=json.loads((src/"sce_sys/param.json").read_text())
assert meta["titleId"]=="PPSA99765"
assert meta["localizedParameters"]["en-US"]["titleName"]=="PS5DreamcastX"
assert "title=PS5DreamcastX" in (src/"build.sh").read_text()
assert "/data/homebrew/PS5DreamcastX/" in (src/"ps5_main.cpp").read_text()
assert 'candidates.push_back("/data/homebrew/PS5DreamcastX");' in (src/"ps5_main.cpp").read_text()
assert 'discoverDreamcastXUsbDirs("/mnt/usb", 8)' in (src/"ps5_main.cpp").read_text()
assert 'bool usb = true;' in (src/"ps5_frontend.h").read_text()
assert '"PS5DreamcastX/"' in (src/"ps5_update.cpp").read_text()
assert 'extern std::string appDir;' in (src/"ps5_frontend.h").read_text()
assert 'const std::string externalData = "/data/PS5DreamcastX";' in (src/"ps5_main.cpp").read_text()
assert 'ps5::rootDir = externalData + "/";' in (src/"ps5_main.cpp").read_text()
assert 'ps5::appDir + "sce_sys/param.json"' in (src/"ps5_main.cpp").read_text()
assert 'swapIn(appDir,' in (src/"ps5_update.cpp").read_text()
assert 'readLine(appDir + "BUILD.txt")' in (src/"ps5_update.cpp").read_text()
assert "AddImageQuad(" not in (src/"dreamcastx_shelf.inc").read_text()
assert "drawCover(dl,g,x0,y0,w,h" in (src/"dreamcastx_shelf.inc").read_text()
assert "PS5_VERSION \"0.1.2\"" in (src/"ps5_build.h").read_text()
feed=(src/"ps5_update.cpp").read_text()
assert "Yahballon/tmp_lap/releases" in feed
assert "ps5-dreamcastx-v" in feed
assert (src/"sce_sys/icon0.png").stat().st_size>1000
print("PASS: PS5DreamcastX core integration, title, paths, branding, safe release filter")
