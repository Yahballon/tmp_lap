#!/usr/bin/env python3
"""Test M12 asset #2201 pinned source workflow/PS5 input and ZIP artifact."""
from pathlib import Path
import hashlib
import importlib.util
import struct
import tempfile
import zipfile

root = Path(__file__).resolve().parents[1] / "samples/m12-platformer-2201"
script = (root/"PS5Controls.gd").read_text()
prep = (root/"prepare_project.py").read_text()
pack = (root/"pack_m12.gd").read_text()
for marker in ("JOY_BUTTON_DPAD_LEFT","JOY_BUTTON_DPAD_RIGHT","JOY_BUTTON_A",
               "JOY_AXIS_LEFT_X", "InputMap.action_add_event", "GODOT_PS5_M12_PLAYER_FOUND",
               "GODOT_PS5_M12_SCENE_READY"):
    assert marker in script, marker
for marker in ("a48fc994cbcb168be472df84f2e13b55d2a2792f",
               "PS5Controls", "GL Compatibility", "gl_compatibility",
               "Level_01.tscn", ".godot/imported", ".ctex", ".sample"):
    assert marker in prep, marker
assert "packer.add_file" in pack and "GODOT_PS5_M12_PCK_WRITTEN" in pack

p=root/"package_platformer.py"
spec=importlib.util.spec_from_file_location("platformer_package",p)
assert spec and spec.loader
module=importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

with tempfile.TemporaryDirectory() as dir:
    d=Path(dir)
    f=d/module.PCK
    content=b"GDPC"+struct.pack("<III",3,4,7)+b"\0"*130000
    f.write_bytes(content)
    lic=d/"LICENSE"
    lic.write_text("MIT License\nCopyright original author\n")
    target,sha=module.pack(f,lic,d/"out")
    assert sha==hashlib.sha256(content).hexdigest()
    assert target.name==module.ZIP
    with zipfile.ZipFile(target) as z:
        assert z.testzip() is None
        assert z.read("GodotPS5/launch.txt")==(module.PCK+"\n").encode()
        assert z.read("GodotPS5/games/"+module.PCK)==content
        assert b"MIT License" in z.read("GodotPS5/M12-Platformer-Original-LICENSE.txt")
    f.write_bytes(b"BAD"+content[3:])
    try:
        module.pack(f,lic,d/"out")
    except ValueError: pass
    else: raise AssertionError("bad PCK accepted")
print("GODOT_PS5_M12_PLATFORMER_STATIC_TEST_PASS")
