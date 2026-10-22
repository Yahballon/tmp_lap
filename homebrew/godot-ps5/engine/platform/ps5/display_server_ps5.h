// SPDX-License-Identifier: MIT
#pragma once
// Early Godot-native PS5 DisplayServer API implementation.
//
// This first compilation gate intentionally uses RasterizerDummy and
// advertises ONLY the "dummy" rendering driver. Native EGL/GLES3 is a
// separate experimental milestone, NOT a capability of this class yet.
#include "servers/display/display_server_headless.h"

class DisplayServerPS5 final : public DisplayServerHeadless {
    GDSOFTCLASS(DisplayServerPS5, DisplayServerHeadless);

    static Vector<String> get_rendering_drivers_func();
    static DisplayServer *create_func(const String &p_rendering_driver,
        DisplayServerEnums::WindowMode p_mode,
        DisplayServerEnums::VSyncMode p_vsync_mode,
        uint32_t p_flags,
        const Vector2i *p_position,
        const Vector2i &p_resolution,
        int p_screen,
        DisplayServerEnums::Context p_context,
        int64_t p_parent_window,
        Error &r_error);

public:
    static void register_ps5_driver();
    String get_name() const override { return "ps5"; }
    int get_screen_count() const override { return 1; }
    int get_primary_screen() const override { return 0; }
    Point2i screen_get_position(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override { return Point2i(); }
    Size2i screen_get_size(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override { return Size2i(1920, 1080); }
    Rect2i screen_get_usable_rect(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override { return Rect2i(Point2i(), Size2i(1920, 1080)); }
    int screen_get_dpi(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override { return 96; }
    float screen_get_refresh_rate(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override { return SCREEN_REFRESH_RATE_FALLBACK; }

    Vector<DisplayServerEnums::WindowID> get_window_list() const override;
    int window_get_current_screen(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override { return 0; }
    Size2i window_get_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override { return Size2i(1920, 1080); }
    Size2i window_get_size_with_decorations(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override { return window_get_size(p_window); }
    // No framebuffer is exposed to the Godot renderer in this build.
    bool window_can_draw(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override { return false; }
    bool can_any_window_draw() const override { return false; }

    DisplayServerPS5() = default;
};
