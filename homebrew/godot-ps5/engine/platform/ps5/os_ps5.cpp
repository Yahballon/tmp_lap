// SPDX-License-Identifier: MIT
// Headless Godot PS5 platform (no graphics, DualSense or audio backend yet).
#include "os_ps5.h"
#include "file_access_ps5.h"
#include "core/os/main_loop.h"
#include "core/os/memory.h"
#include "main/main.h"
#include "servers/display/display_server.h"

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
        controller.poll();
        if (DisplayServer::get_singleton())
            DisplayServer::get_singleton()->process_events();
        if (Main::iteration()) break;
    }
    main_loop->finalize();
}
