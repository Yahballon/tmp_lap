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
shelf=(src/"dreamcastx_shelf.inc").read_text()
# PS5 v0.1.5 broke due to experimental raw ImageQuad/perspective UVs.
# Only native imageFill / rect helpers proven by v0.1.3 are allowed here.
assert "dreamcastXArtworkBounds(" in shelf
assert "drawDreamcastXCase(" in shelf
assert "imageFill(dl,image,art.x,art.y,art.w,art.h" in shelf
for forbidden in ("AddImageQuad(", "AddQuadFilled(", "PrimReserve(", "dreamcastXReflection(", "softDisc("):
    assert forbidden not in shelf, "Unsafe PS5 renderer reintroduced: "+forbidden
assert "ps5-dreamcastx-v" in (src/"ps5_update.cpp").read_text()
assert "PS5_VERSION \"0.1.6\"" in (src/"ps5_build.h").read_text()
async_ui=(root/"build/flycast/core/ui/gui_util.cpp").read_text()
assert '#include "dreamcastx_cover_async.inc"' in async_ui
assert 'dreamcastx_cover_async::texture(path,nearestSampling,textureLoadCount)' in async_ui
assert (root/"build/flycast/core/ui/dreamcastx_cover_async.inc").exists()
assert "for(int radius=0;radius<=2;radius++)" in shelf
feed=(src/"ps5_update.cpp").read_text()
assert "Yahballon/tmp_lap/releases" in feed
assert "ps5-dreamcastx-v" in feed
assert (src/"sce_sys/icon0.png").stat().st_size>1000
print("PASS: PS5DreamcastX core integration, title, paths, branding, safe release filter")
