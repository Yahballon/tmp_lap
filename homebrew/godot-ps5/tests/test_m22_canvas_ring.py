#!/usr/bin/env python3
"""M22: bounded canvas ring; PS5-only, preserves original non-PS5 rendering."""
from pathlib import Path
p=Path(__file__).resolve().parents[1]
script=(p/"scripts/patch_ps5_m22_bounded_canvas_ring.py").read_text()
build=(p/"scripts/build-engine.sh").read_text()
launcher=(p/"native/gles3_canvas_main.cpp").read_text()
for q in (
 "GODOT_PS5_M22_BOUNDED_CANVAS_RING",
 "m22_max_canvas_sets=9", 
 "state.canvas_instance_data_buffers.size() >= m22_max_canvas_sets",
 "glBufferData(GL_ARRAY_BUFFER,data.max_instance_buffer_size,nullptr,GL_STREAM_DRAW)",
 "glDeleteSync(db.fence)",
 "GL_MAP_UNSYNCHRONIZED_BIT",
 "state.current_data_buffer_index",
 "db.last_frame_used=RSG::rasterizer->get_frame_number()",
 "std::printf",
 "#if defined(PS5_ENABLED) && defined(GODOT_PS5_NATIVE_GLES3)",
):
    assert q in script,q
assert "GODOT_PS5_M22_BOUNDED_RING" in build
assert "patch_ps5_m22_bounded_canvas_ring.py" in build
assert 'GODOT_PS5_M22_BOUNDED_CANVAS_RING active=1 max_sets=9 orphan=1' in launcher
assert 'PS5PresentMode perf_present_mode = PS5PresentMode::VSync;' in launcher
assert 'GODOT_PS5_M19_GL_SUMMARY_5S' in launcher
assert 'GODOT_PS5_M21_CANVAS_FENCE_NONBLOCKING active=1' in launcher
print("GODOT_PS5_M22_BOUNDED_RING_SOURCE_TEST_PASS")
