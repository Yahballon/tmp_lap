#!/usr/bin/env python3
"""M21 strict upstream GLES3 canvas-fence query patch regression gates."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
s=(root/"scripts/patch_ps5_m21_canvas_fence.py").read_text()
build=(root/"scripts/build-engine.sh").read_text()
native=(root/"native/gles3_canvas_main.cpp").read_text()
for x in ("GODOT_PS5_M21_NONBLOCKING_CANVAS_FENCE","glGetSynciv(",
    "glClientWaitSync(","GL_ALREADY_SIGNALED","GL_CONDITION_SATISFIED",
    "GL_UNSIGNALED","GL_WAIT_FAILED","0, 0);",
    "Unexpected pinned upstream Godot canvas fence query",
    "source partially patched"):
    assert x in s,x
assert "GODOT_PS5_M21_NONBLOCKING_CANVAS_FENCE" in build
assert "patch_ps5_m21_canvas_fence.py" in build
assert "GODOT_PS5_NATIVE_GLES3" in build
assert "GODOT_PS5_M21_CANVAS_FENCE_NONBLOCKING active=1" in native
assert "PS5PresentMode perf_present_mode = PS5PresentMode::VSync;" in native
assert "GODOT_PS5_M19_GL_5S" in native
assert "GODOT_PS5_M20_GPU_TIMESTAMP_SYNC_DISABLED active=1" in native
assert "GODOT_PS5_M17_PHASE_5S" in (root/"scripts/patch_ps5_m17_iteration.py").read_text()
print("GODOT_PS5_M21_CANVAS_FENCE_SAFE_AB_GUARDS_PASS")
