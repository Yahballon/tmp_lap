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
assert "PPSA99765" in (src/"build.sh").read_text()
assert "PS5_VERSION \"0.1.0\"" in (src/"ps5_build.h").read_text()
feed=(src/"ps5_update.cpp").read_text()
assert "Yahballon/tmp_lap/releases" in feed
assert "ps5-dreamcastx-v" in feed
assert (src/"sce_sys/icon0.png").stat().st_size>1000
print("PASS: PS5DreamcastX core integration, title, paths, branding, safe release filter")
