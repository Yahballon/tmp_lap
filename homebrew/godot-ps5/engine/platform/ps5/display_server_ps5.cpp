// SPDX-License-Identifier: MIT
// First native Godot DisplayServerPS5 registration/ABI validation stage.
// This is NOT an EGL-enabled Godot renderer. GLES3 is deliberately disabled.
#include "display_server_ps5.h"

#include "core/error/error_macros.h"
#include "core/os/memory.h"
#include "servers/rendering/dummy/rasterizer_dummy.h"

Vector<String> DisplayServerPS5::get_rendering_drivers_func() {
    Vector<String> drivers;
    drivers.push_back("dummy");
    return drivers;
}

DisplayServer *DisplayServerPS5::create_func(
        const String &p_rendering_driver,
        DisplayServerEnums::WindowMode p_mode,
        DisplayServerEnums::VSyncMode p_vsync_mode,
        uint32_t p_flags,
        const Vector2i *p_position,
        const Vector2i &p_resolution,
        int p_screen,
        DisplayServerEnums::Context p_context,
        int64_t p_parent_window,
        Error &r_error) {
    // Refuse to present an unimplemented EGL renderer as successful.
    if (!p_rendering_driver.is_empty() && p_rendering_driver != "dummy") {
        r_error = ERR_UNAVAILABLE;
        ERR_PRINT("PS5 DisplayServer: EGL/GLES3 not wired; refusing non-dummy renderer.");
        return nullptr;
    }
    r_error = OK;
    RasterizerDummy::make_current();
    // The separate PS5 GPU probe proved a fixed 1080p GL surface; this
    // class only exposes a logical window and cannot draw to it yet.
    print_line("[GodotPS5] DISPLAY_SERVER_PS5_DUMMY_REGISTERED_NO_VIDEO");
    return memnew(DisplayServerPS5);
}

Vector<DisplayServerEnums::WindowID> DisplayServerPS5::get_window_list() const {
    Vector<DisplayServerEnums::WindowID> windows;
    windows.push_back(DisplayServerEnums::MAIN_WINDOW_ID);
    return windows;
}

void DisplayServerPS5::register_ps5_driver() {
    DisplayServer::register_create_function("ps5", create_func, get_rendering_drivers_func);
}
