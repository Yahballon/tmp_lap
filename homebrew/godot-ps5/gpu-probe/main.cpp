// SPDX-License-Identifier: GPL-3.0-or-later
// Experimental stand-alone PS5 OpenGL/EGL bring-up.
// This is deliberately NOT linked to or distributed as Godot's MIT engine.
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "core/input.hpp"
#include <EGL/egl.h>
#include <GL/glcorearb.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

namespace {
constexpr char kLog[] = "/data/GodotPS5/graphics-probe.log";

void record(const char *event, int value = -1) {
    if (FILE *f = std::fopen(kLog, "ab")) {
        if (value >= 0) std::fprintf(f, "[GodotPS5 GPU 0.1.0] %s=%d\n", event, value);
        else std::fprintf(f, "[GodotPS5 GPU 0.1.0] %s\n", event);
        std::fflush(f);
        std::fclose(f);
    }
}

void record_string(const char *tag, const GLubyte *text) {
    if (FILE *f = std::fopen(kLog, "ab")) {
        std::fprintf(f, "[GodotPS5 GPU 0.1.0] %s=%s\n", tag,
                     text ? reinterpret_cast<const char *>(text) : "(null)");
        std::fflush(f);
        std::fclose(f);
    }
}
} // namespace

int main() {
    mkdir("/data/GodotPS5", 0755);
    record("BOOT");
    hui::ps5::Display display;
    if (!display.open(1920, 1080)) {
        record("EGL_OPEN_FAILED", static_cast<int>(display.last_error()));
        hui::sys::quit();
    }
    record("EGL_OPEN_OK");
    record("SURFACE_WIDTH", display.width());
    record("SURFACE_HEIGHT", display.height());
    record_string("GL_VERSION", glGetString(GL_VERSION));
    record_string("GL_RENDERER", glGetString(GL_RENDERER));
    record_string("GLSL_VERSION", glGetString(GL_SHADING_LANGUAGE_VERSION));

    hui::ps5::Pad pad;
    record(pad.open() ? "PAD_OPEN_OK" : "PAD_UNAVAILABLE");
    std::uint32_t previous = 0;
    int selected = 0;
    bool splash_hidden = false;
    int presented = 0;

    // A controlled GPU-only test: clear+present real framebuffer colors.
    // It requires no Godot DisplayServer or GDScript and stays separately packaged.
    constexpr std::array<std::array<float, 3>, 3> colors = {{
        {{0.04f, 0.20f, 0.92f}}, // Blue
        {{0.04f, 0.78f, 0.24f}}, // Green
        {{0.92f, 0.12f, 0.18f}}  // Red
    }};
    for (int frame = 0; frame < 1800; ++frame) {
        std::array<hui::PadSample, 64> samples{};
        std::uint32_t pressed = 0;
        for (size_t i = 0, n = pad.read(samples); i < n; ++i) {
            pressed |= samples[i].buttons & ~previous;
            previous = samples[i].buttons;
        }
        if (pressed & hui::pad_bits::kCircle) {
            record("CIRCLE_EXIT");
            break;
        }
        if (pressed & hui::pad_bits::kCross) {
            selected = (selected + 1) % 3;
            record("CROSS_COLOR", selected);
        }
        const int index = (selected + (frame / 120)) % 3;
        const auto &color = colors[index];
        glViewport(0, 0, display.width(), display.height());
        glClearColor(color[0], color[1], color[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (!display.swap()) {
            record("EGL_SWAP_FAILED", static_cast<int>(display.last_error()));
            break;
        }
        ++presented;
        if (!splash_hidden) {
            hui::sys::hide_splash_screen();
            splash_hidden = true;
            record("FIRST_FRAME_PRESENTED");
        }
        if (presented == 60 || presented == 120 || presented == 600)
            record("PRESENTED_FRAME_COUNT", presented);
        hui::sys::sleep_us(1000);
    }
    record("TOTAL_PRESENTED", presented);
    pad.close();
    display.close();
    record("CLEAN_SHUTDOWN");
    hui::sys::quit();
}
