#!/usr/bin/env python3
"""Apply AndroidFlow's Android touchscreen bridge to Hydroid Godot 3 host.

Hydroid's Godot JNI host currently consumes only key and joystick events; unlike
Unity/Cocos, it has no touch callback, and the app_main.c Godot path never turns
pad_touch_events on. This patch supplies both v3.2 jint[] and v3.5 jfloat[]
native touch APIs, a DualSense right-stick cursor and Cross/touchpad taps.
The pinned Hydroid source is edited at build time; upstream is unchanged.
"""
from pathlib import Path
import argparse
import shutil

def replace_once(text, old, new, where):
    if text.count(old) != 1:
        raise RuntimeError(f"{where} patch anchor changed ({text.count(old)}): {old[:78]!r}")
    return text.replace(old, new, 1)

HELPERS = r'''
// AndroidFlow touch support: native Godot 3 JNI touch (legacy and modern).
// The existing Hydroid GL bridge already draws droid_cursor_shown on every
// swap; reuse its exact pointer instead of introducing another GL context.
#include "yah_godot_cursor.h"
extern float droid_cursor_x, droid_cursor_y;
extern volatile int droid_cursor_shown;
static yah_godot_cursor touch_pointer = { .x = 960.f, .y = 540.f };
static int godot_cross_down, godot_pad_down;
static int godot_cursor_mode = 1; // R3 toggles touchscreen cursor vs gamepad
static int godot_trace_frames;

static int godot_pad_x, godot_pad_y;
static int godot_touch_sent;
// Hydroid's GL cursor overlay became permanently painted into Godot's
// preserved backbuffers. Clear default framebuffer before Godot draws, while
// restoring the scissor, FBO, color mask and clear-color state after the pass.
static void godot_clear_previous_cursor(void) {
    typedef void (*p_getint)(unsigned, int *);
    typedef void (*p_getfloat)(unsigned, float *);
    typedef void (*p_getbool)(unsigned, unsigned char *);
    typedef void (*p_bind)(unsigned, unsigned);
    typedef void (*p_toggle)(unsigned);
    typedef void (*p_mask)(unsigned char,unsigned char,unsigned char,unsigned char);
    typedef void (*p_color)(float,float,float,float);
    typedef void (*p_clear)(unsigned);
    static struct {
        p_getint getint; p_getfloat getfloat; p_getbool getbool;
        p_bind bind; p_toggle enable, disable; p_mask mask;
        p_color color; p_clear clear;
    } gl;
    static int initialized, supported;
    if (!initialized) {
        initialized = 1;
        gl.getint = (p_getint)droid_resolve("glGetIntegerv");
        gl.getfloat = (p_getfloat)droid_resolve("glGetFloatv");
        gl.getbool = (p_getbool)droid_resolve("glGetBooleanv");
        gl.bind = (p_bind)droid_resolve("glBindFramebuffer");
        gl.enable = (p_toggle)droid_resolve("glEnable");
        gl.disable = (p_toggle)droid_resolve("glDisable");
        gl.mask = (p_mask)droid_resolve("glColorMask");
        gl.color = (p_color)droid_resolve("glClearColor");
        gl.clear = (p_clear)droid_resolve("glClear");
        supported = gl.getint && gl.getfloat && gl.getbool && gl.bind &&
                    gl.enable && gl.disable && gl.mask && gl.color && gl.clear;
        droid_log("androidflow: Godot full-frame cursor cleanup %s",
                  supported ? "enabled" : "unavailable");
    }
    if (!supported) return;
    int old_draw = 0, old_read = 0;
    float old_clear[4];
    unsigned char old_mask[4], old_scissor;
    gl.getint(0x8CA6u, &old_draw);     // GL_DRAW_FRAMEBUFFER_BINDING
    gl.getint(0x8CAAu, &old_read);     // GL_READ_FRAMEBUFFER_BINDING
    gl.getfloat(0x0C22u, old_clear);  // GL_COLOR_CLEAR_VALUE
    gl.getbool(0x0C23u, old_mask);    // GL_COLOR_WRITEMASK
    gl.getbool(0x0C11u, &old_scissor); // GL_SCISSOR_TEST
    gl.bind(0x8D40u, 0);             // GL_FRAMEBUFFER
    gl.disable(0x0C11u);            // GL_SCISSOR_TEST
    gl.mask(1,1,1,1);
    gl.color(0.f,0.f,0.f,1.f);
    gl.clear(0x4000u);              // GL_COLOR_BUFFER_BIT
    gl.color(old_clear[0],old_clear[1],old_clear[2],old_clear[3]);
    gl.mask(old_mask[0],old_mask[1],old_mask[2],old_mask[3]);
    if (old_scissor) gl.enable(0x0C11u);
    gl.bind(0x8CA9u, (unsigned)old_draw); // GL_DRAW_FRAMEBUFFER
    gl.bind(0x8CA8u, (unsigned)old_read); // GL_READ_FRAMEBUFFER
}

static int godot_touch_supported(void) {
    return lib.touch_modern != NULL || lib.touch_legacy != NULL;
}
static void godot_touch_send(int action, int x, int y) {
    if (!godot_touch_supported()) return;
    x = (int)yah_pointer_clamp(x, 0, 1919);
    y = (int)yah_pointer_clamp(y, 0, 1079);
    if (godot_touch_sent < 32)
        droid_log("androidflow: touch JNI BEFORE action=%d x=%d y=%d", action, x, y);
    if (action == 0 || action == 1) droid_log_flush(); // before risky JNI call
    // Exactly one active finger; UP includes its last location. JNI has arrays
    // of [pointer_id,x,y], and the array handle is bridged to ARM by FalsoJNI.
    if (lib.touch_modern) {
        jfloatArray positions = jni->NewFloatArray(&jni, 3);
        if (!positions) return;
        const jfloat xyz[3] = { 0.f, (jfloat)x, (jfloat)y };
        jni->SetFloatArrayRegion(&jni, positions, 0, 3, xyz);
        lib.touch_modern(env, clazz, 0x1002, action, 0, 1, positions);
        jni->DeleteLocalRef(&jni, positions);
    } else {
        jintArray positions = jni->NewIntArray(&jni, 3);
        if (!positions) return;
        const jint xyz[3] = { 0, x, y };
        jni->SetIntArrayRegion(&jni, positions, 0, 3, xyz);
        lib.touch_legacy(env, clazz, action, 0, 1, positions);
        jni->DeleteLocalRef(&jni, positions);
    }
    if (action == 0 || action == 1) droid_log_flush(); // JNI call returned
    if (action == 1) godot_trace_frames = 120;
    if (godot_touch_sent++ < 32)
        droid_log("androidflow: Godot touch %s at %d,%d (%s)", action == 0 ? "DOWN" :
                  action == 1 ? "UP" : "MOVE", x, y,
                  lib.touch_modern ? "Godot 3.5" : "Godot 3.2");
}
static void godot_touchpad_input(const droid_input_event *e) {
    int n = (int)e->axes[0];
    if (n > 0) {
        int x = yah_pointer_coord(e->axes[1], 1920);
        int y = yah_pointer_coord(e->axes[2], 1080);
        if (!godot_pad_down) {
            // Do not overlap two independent virtual fingers using pointer id 0.
            if (godot_cross_down) {
                godot_touch_send(1, (int)touch_pointer.x, (int)touch_pointer.y);
                godot_cross_down = 0;
            }
            godot_touch_send(0, x, y);
            godot_pad_down = 1;
        } else if (godot_pad_x != x || godot_pad_y != y) {
            godot_touch_send(2, x, y);
        }
        godot_pad_x = x;
        godot_pad_y = y;
        touch_pointer.x = droid_cursor_x = (float)x;
        touch_pointer.y = droid_cursor_y = (float)y;
        droid_cursor_shown = godot_cursor_mode;
    } else if (godot_pad_down) {
        godot_touch_send(1, godot_pad_x, godot_pad_y);
        godot_pad_down = 0;
    }
}
static void godot_pointer_tick(float dt) {
    if (!godot_cursor_mode) return;
    if (yah_pointer_tick(&touch_pointer, dt)) {
        droid_cursor_x = touch_pointer.x;
        droid_cursor_y = touch_pointer.y;
        droid_cursor_shown = 1;
        if (godot_cross_down && !godot_pad_down)
            godot_touch_send(2, (int)touch_pointer.x, (int)touch_pointer.y);
    }
}
'''
# C host calls the JNI native function via arm_callback, not through Java UI thread.
# Render loop runs one native thread; no extra synchronization is needed.
def patch(root: Path, assets: Path) -> None:
    host = root / "src/godot_host.c"
    app = root / "src/app_main.c"
    h = host.read_text()
    h = replace_once(h,
        '    void (*key)(void *, void *, int32_t, int32_t, uint8_t);',
        '    void (*touch_legacy)(void *, void *, int32_t, int32_t, int32_t, jintArray);\n'
        '    void (*touch_modern)(void *, void *, int32_t, int32_t, int32_t, int32_t, jfloatArray);\n'
        '    void (*key)(void *, void *, int32_t, int32_t, uint8_t);', host)
    h = replace_once(h,
        'static float last_axis[8];',
        HELPERS + '\nstatic float last_axis[8];', host)
    h = replace_once(h,
        '    if (e->type == 1) { // key\n        uint8_t down = e->action == 0;',
        '    if (e->type == DROID_EVENT_TOUCH) {\n'
        '        godot_touchpad_input(e);\n'
        '        return;\n'
        '    }\n'
        '    if (e->type == 1) { // key\n'
        '        uint8_t down = e->action == 0;\n'
        '        if (e->keycode == 96 && godot_touch_supported() && !godot_pad_down) {\n'
        '            if (down && !godot_cross_down) {\n'
        '                godot_cross_down = 1;\n'
        '                godot_touch_send(0, (int)touch_pointer.x, (int)touch_pointer.y);\n'
        '                droid_cursor_shown = 1;\n'
        '            } else if (!down && godot_cross_down) {\n'
        '                godot_touch_send(1, (int)touch_pointer.x, (int)touch_pointer.y);\n'
        '                godot_cross_down = 0;\n'
        '            }\n'
        '        }', host)
    h = replace_once(h,
        '    axis(0, e->axes[0]);   // left stick',
        '    if (e->type != 2) return; // only actual joystick motion reaches axes\n'
        '    touch_pointer.dx = e->axes[11];\n'
        '    touch_pointer.dy = e->axes[14];\n'
        '    axis(0, e->axes[0]);   // left stick', host)
    h = replace_once(h,
        '    for (uint64_t frame = 0;; frame++) {',
        '    double last_cursor_ms = now_ms();\n'
        '    for (uint64_t frame = 0;; frame++) {', host)
    h = replace_once(h,
        '        if (frame >= 2) poll_input(godot_input);\n',
        '        if (frame >= 2) {\n'
        '            poll_input(godot_input);\n'
        '            double cursor_now = now_ms();\n'
        '            godot_pointer_tick((float)((cursor_now - last_cursor_ms) / 1000.0));\n'
        '            last_cursor_ms = cursor_now;\n'
        '        }\n', host)
    h = replace_once(h,
        '    lib.key = native("key");',
        '    // Godot 3.5+ uses float[] JNI long-form mangling, older Godot 3 uses int[].\n'
        '    lib.touch_modern = so_sym(game, LIB "touch__IIII_3F") ? native("touch__IIII_3F") : NULL;\n'
        '    lib.touch_legacy = !lib.touch_modern && so_sym(game, LIB "touch") ? native("touch") : NULL;\n'
        '    droid_log("androidflow: Godot touchscreen: %s", lib.touch_modern ? "modern v3.5" :\n'
        '              lib.touch_legacy ? "legacy v3.2" : "unavailable");\n'
        '    lib.key = native("key");', host)

    # In cursor mode, Cross must produce only ONE input method. Before, a
    # single Cross simultaneously fired touch DOWN/UP, Enter and Joypad A;
    # some Godot menus handle these as duplicate scene transitions.
    h = replace_once(h,
        '        if (e->keycode == 96 && godot_touch_supported() && !godot_pad_down) {',
        '        if (e->keycode == 107 && godot_touch_supported()) {\n'
        '            if (down) {\n'
        '                if (godot_cross_down) {\n'
        '                    godot_touch_send(1, (int)touch_pointer.x, (int)touch_pointer.y);\n'
        '                    godot_cross_down = 0;\n'
        '                }\n'
        '                godot_cursor_mode = !godot_cursor_mode;\n'
        '                droid_cursor_shown = 0;\n'
        '                droid_log("androidflow: Godot cursor mode %s", godot_cursor_mode ? "ON" : "OFF");\n'
        '            }\n'
        '            return; // R3 only toggles mode, no joystick event\n'
        '        }\n'
        '        if (e->keycode == 96 && godot_cursor_mode && godot_touch_supported()) {', host)
    h = replace_once(h,
        '                godot_cross_down = 0;\n            }\n        }\n        int b = joy_button(e->keycode);',
        '                godot_cross_down = 0;\n'
        '            }\n'
        '            return; // avoid triple Play click: touch + Enter + joypad A\n'
        '        }\n'
        '        int b = joy_button(e->keycode);', host)
    # The previous arrow was drawn into the retained Godot backbuffer.
    # A full default-surface color clear removes old arrows before every step.
    h = replace_once(h,
        '        double t0 = now_ms();\n        lib.step(env, clazz);\n        double t1 = now_ms();',
        '        godot_clear_previous_cursor();\n'
        '        int trace = godot_trace_frames > 0 && godot_trace_frames % 12 == 0;\n'
        '        if (trace) {\n'
'            droid_log("androidflow: scene BEFORE step=%llu", (unsigned long long)frame);\n'
'            droid_log_flush();\n'
'        }\n'
        '        double t0 = now_ms();\n'
        '        lib.step(env, clazz);\n'
        '        double t1 = now_ms();\n'
        '        if (trace) droid_log("androidflow: scene AFTER step=%llu (%.1f ms)",\n'
        '                             (unsigned long long)frame, t1 - t0);\n'
        '        if (godot_trace_frames > 0) godot_trace_frames--;', host)

    host.write_text(h)
    a = app.read_text()
    a = replace_once(a,
        '            say(1, "Android: Godot game from the APK");\n            godot_run(m, pad_poll);',
        '            say(1, "Android: Godot game from the APK");\n'
        '            pad_touch_events = 1; // AndroidFlow: Godot menus require real touchscreen events\n'
        '            godot_run(m, pad_poll);', app)
    app.write_text(a)
    shutil.copyfile(assets / "native/yah_godot_cursor.h", root / "src/yah_godot_cursor.h")
    print("AndroidFlow: Godot touchscreen and cursor patched into pinned Hydroid")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--assets", type=Path, required=True)
    args = parser.parse_args()
    patch(args.source, args.assets)

if __name__ == "__main__":
    main()
