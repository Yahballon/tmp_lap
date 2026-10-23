// SPDX-License-Identifier: MIT
// Native Godot runtime + DualSense. Graphics remain opt-in and experimental.
#include "os_ps5.h"
#include "file_access_ps5.h"
#include "core/os/main_loop.h"
#include "core/os/memory.h"
#include "main/main.h"
#include "servers/display/display_server.h"
#ifdef GODOT_PS5_SCENE_PREVIEW
#include "scene/main/scene_tree.h"
#include "scene/2d/node_2d.h"
#include "scene_frame_hooks_ps5.h"
#include <cstdio>
// Registered by the experimental PPSA99845 launcher; never used by the
// real Godot GLES3 backend or the known-good headless title.
static GodotPS5SceneFrameCallback scene_frame_callback = nullptr;
extern "C" void godot_ps5_set_scene_frame_callback(GodotPS5SceneFrameCallback cb) {
    scene_frame_callback = cb;
}
#endif
#ifdef GODOT_PS5_DISPLAY_EXPERIMENT
#include "display_server_ps5.h"
#endif

OS_PS5::OS_PS5() {
#ifdef GODOT_PS5_AUDIOOUT
    // Before Main::setup() selects a driver; fail closed if AudioOut cannot open.
    AudioDriverManager::add_driver(&audio_driver);
    print_line("GODOT_PS5_M11C_AUDIOOUT_DRIVER_REGISTERED");
#endif
#ifdef GODOT_PS5_DISPLAY_EXPERIMENT
    // Godot registers graphical backends in the platform OS constructor,
    // before Main::setup() picks the display driver.
    DisplayServerPS5::register_ps5_driver();
#endif
}

void OS_PS5::initialize() {
    OS_Unix::initialize_core();
    // Godot's resource/PCK reads must use the PS5 kernel API rather than
    // FreeBSD libc fopen/fseeko, which isn't reliable on app0 on hardware.
    FileAccess::make_default<FileAccessPS5>(FileAccess::ACCESS_RESOURCES);
    FileAccess::make_default<FileAccessPS5>(FileAccess::ACCESS_FILESYSTEM);
}
void OS_PS5::initialize_joypads() {
    controller.initialize();
}
bool OS_PS5::_check_internal_feature_support(const String &feature) {
    (void)feature;
    return false; // Do not advertise unimplemented PS5 features.
}
void OS_PS5::finalize() {
    controller.shutdown();
    delete_main_loop();
}
void OS_PS5::delete_main_loop() {
    if (main_loop) memdelete(main_loop);
    main_loop = nullptr;
}
void OS_PS5::set_main_loop(MainLoop *loop) { main_loop = loop; }
MainLoop *OS_PS5::get_main_loop() const { return main_loop; }
// Godot's FreeBSD getcwd() crashes on PS5 firmware tested in v0.2.4.
String OS_PS5::get_cwd() const { return "/app0"; }
String OS_PS5::get_executable_path() const { return "/app0/eboot.bin"; }
Error OS_PS5::set_cwd(const String &path) {
    // Logical app root only; never call unverified libc chdir().
    return path == "/app0" ? OK : ERR_UNAVAILABLE;
}
String OS_PS5::get_identifier() const { return "ps5"; }
String OS_PS5::get_name() const { return "PS5 Homebrew"; }
String OS_PS5::get_config_path() const { return "/data/GodotPS5/config"; }
String OS_PS5::get_data_path() const { return "/data/GodotPS5"; }
String OS_PS5::get_cache_path() const { return "/data/GodotPS5/cache"; }
String OS_PS5::get_user_data_dir(const String &user) const {
    return String("/data/GodotPS5/userdata").path_join(user.validate_filename());
}
void OS_PS5::run() {
    if (!main_loop) return;
    main_loop->initialize();
    while (true) {
#ifdef GODOT_PS5_SCENE_PREVIEW
        // Capture *real* frame costs without synchronous per-button logging.
        const uint64_t frame_start_us = get_ticks_usec();
#endif
        controller.poll();
#ifdef GODOT_PS5_SCENE_PREVIEW
        const uint64_t after_pad_us = get_ticks_usec();
#endif
        if (DisplayServer::get_singleton())
            DisplayServer::get_singleton()->process_events();
        const bool done = Main::iteration();
#ifdef GODOT_PS5_SCENE_PREVIEW
        const uint64_t after_main_us = get_ticks_usec();
        const uint32_t pad_poll_us = static_cast<uint32_t>(
                MIN(after_pad_us - frame_start_us, uint64_t(10000000)));
        const uint32_t main_loop_us = static_cast<uint32_t>(
                MIN(after_main_us - after_pad_us, uint64_t(10000000)));
#endif
#ifdef GODOT_PS5_SCENE_PREVIEW
        if (scene_frame_callback) {
            SceneTree *tree = Object::cast_to<SceneTree>(main_loop);
            Node *scene = tree ? tree->get_current_scene() : nullptr;
            if (scene) {
                Node2D *sprite = Object::cast_to<Node2D>(
                    scene->get_node_or_null(NodePath("PreviewSprite")));
                if (sprite) {
                    const int presses = int(scene->get("count"));
                    scene_frame_callback(sprite->get_position().x,
                                         sprite->get_rotation(), presses,
                                         main_loop_us, pad_poll_us);
                }
            }
        }
#endif
        if (done) break;
    }
    main_loop->finalize();
}
