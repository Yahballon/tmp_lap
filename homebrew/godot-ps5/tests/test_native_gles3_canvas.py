#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
native=(root/"native/gles3_canvas_main.cpp").read_text()
workflow=(root.parent.parent/".github/workflows/build-godot-ps5-gles3-canvas.yml").read_text()
for token in ("godot_ps5_set_graphics_hooks(&hooks)", "godot_ps5_run_pack(",
              "BOOT_FRAME_PRESENTED","GODOT_SWAP_FIRST","gles3-stderr.log","gl_resolve"):
    assert token in native,token
for token in ("GODOT_PS5_NATIVE_GLES3: '1'","GODOT_PS5_GLES3_ARCHIVES: '1'",
              "GODOT_PS5_DISPLAY_EXPERIMENT: '1'","PPSA99847",
              "visual-smoke.pck","graphics_hooks_ps5.h","stage_native_link.py"):
    assert token in workflow,token

assert "glDrawArrays(" not in native
assert "glTexImage2D(" not in native
assert "gh release create" not in workflow
for symbol in ('#include "pck_launch_config.hpp"', 'read_launch_txt()',
               'trusted_external_path(requested)', 'valid_godot4_pack(requested)',
               'PACK_SOURCE=%s PACK=%s', 'BUNDLED_VISUAL', 'visual-smoke.pck'):
    assert symbol in native, symbol
assert 'first_pack_in(' not in native, "Do not auto-load stale external content"
assert "Godot-PS5-REAL-GLES3-Canvas-v0.1.1-" in workflow
assert "contentVersion='01.000.002'" in workflow
print("PASS: true Godot GLES3 Canvas2D PS5 title source/build isolated")
