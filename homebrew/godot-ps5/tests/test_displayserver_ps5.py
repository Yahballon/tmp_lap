#!/usr/bin/env python3
"""Host guardrails for real Godot-PS5 DisplayServer ABI bringup."""
from pathlib import Path
p = Path(__file__).resolve().parents[1]
overlay = p / "engine/platform/ps5"
header = (overlay / "display_server_ps5.h").read_text()
impl = (overlay / "display_server_ps5.cpp").read_text()
os_src = (overlay / "os_ps5.cpp").read_text()
scons = (overlay / "SCsub").read_text()
detect = (overlay / "detect.py").read_text()
for symbol in ("public DisplayServerHeadless", "get_screen_count() const override",
               "get_screen_size(", "window_can_draw(", "get_rendering_drivers_func",
               "register_ps5_driver"):
    if symbol == "get_screen_size(":
        assert "screen_get_size(" in header
    else:
        assert symbol in header, symbol
assert '"ps5", create_func' in impl
assert 'RasterizerDummy::make_current()' in impl
assert 'ERR_UNAVAILABLE' in impl
assert 'p_rendering_driver != "dummy"' in impl
assert 'DISPLAY_SERVER_PS5_DUMMY_REGISTERED_NO_VIDEO' in impl
assert 'DisplayServerPS5::register_ps5_driver()' in os_src
assert '#ifdef GODOT_PS5_DISPLAY_EXPERIMENT' in os_src
assert 'GODOT_PS5_DISPLAY_EXPERIMENT' in detect
assert 'GODOT_PS5_DISPLAY_EXPERIMENT' in scons
assert 'opengl3": False' in detect
assert 'return false;' in header
assert 'EGL' not in (overlay / "os_ps5.h").read_text()
print("PASS: isolated DisplayServerPS5 ABI target does not claim OpenGL.")
