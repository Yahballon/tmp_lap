#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
native=(root/"native/gles3_canvas_main.cpp").read_text()
workflow=(root.parent.parent/".github/workflows/build-godot-ps5-gles3-canvas.yml").read_text()
for token in ("godot_ps5_set_graphics_hooks(&hooks)", "godot_ps5_run_pack(",
              "BOOT_FRAME_PRESENTED","GODOT_SWAP_FIRST","gles3-stderr.log","gl_resolve"):
    assert token in native,token
for token in ("GODOT_PS5_NATIVE_GLES3: '1'","GODOT_PS5_GLES3_ARCHIVES: '1'",
              "GODOT_PS5_DISPLAY_EXPERIMENT: '1'","PPSA99846",
              "visual-smoke.pck","graphics_hooks_ps5.h","stage_native_link.py"):
    assert token in workflow,token
import re
assert re.search(r"\\bglDrawArrays\\s*\\(", native) is None
assert re.search(r"\\bglTexImage2D\\s*\\(", native) is None
assert "gh release create" not in workflow
print("PASS: true Godot GLES3 Canvas2D PS5 title source/build isolated")
