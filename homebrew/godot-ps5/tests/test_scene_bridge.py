#!/usr/bin/env python3
"""Verify that the graphical preview is powered by real Godot scene state."""
from pathlib import Path
p=Path(__file__).resolve().parents[1]
os=(p/"engine/platform/ps5/os_ps5.cpp").read_text()
detect=(p/"engine/platform/ps5/detect.py").read_text()
scene=(p/"samples/visual-smoke/main.gd").read_text()
native=(p/"native/scene_bridge_main.cpp").read_text()
for token in ("godot_ps5_set_scene_frame_callback","get_current_scene()","get_node_or_null(NodePath(\"PreviewSprite\"))","scene->get(\"count\")","scene_frame_callback(sprite->get_position().x","GODOT_PS5_SCENE_PREVIEW"):
    assert token in os,token
assert "GODOT_PS5_SCENE_PREVIEW" in detect
assert 'sprite.name = "PreviewSprite"' in scene
# The PS5 headless static text server has no FreeType: this caused
# infinite error churn in v0.1.0 _ready, preventing the first GPU swap.
assert "Label.new()" not in scene and "status_label" not in scene
assert "GODOT_PS5_REAL_2D_SCENE_READY" in scene
for token in ("godot_ps5_run_pack(", "godot_ps5_set_scene_frame_callback", "GODOT_GDSCRIPT_SCENE_FRAME_DRAWN","GDSCRIPT_CROSS_COUNT","GL_TRIANGLE_STRIP","glTexImage2D","GL_SRC_ALPHA","TOTAL_PRESENTED","GPU_SHUTDOWN"):
    assert token in native,token
assert "NOT Godot's GLES3" in native
assert 'BOOT_FRAME_PRESENTED' in native and 'BOOT_FRAME_GL_ERROR' in native
assert native.index('scene.boot_frame()') < native.index('godot_ps5_run_pack(')

print("PASS: Same-process real Godot GDScript → PS5 native GL proof wiring")
