#!/usr/bin/env python3
"""M20: PS5-only disabling the *synchronous GPU profiling queries* must be isolated."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
patch=(root/"scripts/patch_ps5_m20_gl_timestamps.py").read_text()
build=(root/"scripts/build-engine.sh").read_text()
native=(root/"native/gles3_canvas_main.cpp").read_text()
m17=(root/"scripts/patch_ps5_m17_iteration.py").read_text()
for marker in ("GODOT_PS5_M20_GPU_TIMESTAMPS_DISABLED",
               "glGetQueryObjectui64v(", "glQueryCounter(",
               "GL_QUERY_RESULT", "GL_TIMESTAMP",
               "defined(PS5_ENABLED) && defined(GODOT_PS5_NATIVE_GLES3)",
               "source.count(anchor1) != 1", "source.count(anchor2) != 1"):
    assert marker in patch, marker
assert "GODOT_PS5_M20_DISABLE_GL_QUERIES" in build
assert "patch_ps5_m20_gl_timestamps.py" in build
assert "GODOT_PS5_NATIVE_GLES3" in build
assert "GODOT_PS5_M20_GPU_TIMESTAMP_SYNC_DISABLED active=1" in native
assert "PS5PresentMode perf_present_mode = PS5PresentMode::VSync;" in native
assert "GODOT_PS5_M19_GL_SUMMARY_5S" in native
assert "GODOT_PS5_M17_PHASE_5S" in m17
assert "glGetQueryObjectui64v(" not in native  # not fake-intercepted
assert "glQueryCounter(" not in native
print("GODOT_PS5_M20_SAFE_GPU_TIMER_QUERY_BYPASS_GUARDS_PASS")
