// SPDX-License-Identifier: MIT
// Headless Godot PS5 platform (no graphics, DualSense or audio backend yet).
#include "os_ps5.h"
#include "core/os/memory.h"
#include "main/main.h"
#include "servers/display/display_server.h"

void OS_PS5::initialize() { OS_Unix::initialize_core(); }
void OS_PS5::initialize_joypads() {
    // Native pad-to-Godot Input bridge will be implemented later.
}
bool OS_PS5::_check_internal_feature_support(const String &feature) {
    (void)feature;
    return false; // Do not advertise unimplemented PS5 features.
}
void OS_PS5::finalize() { delete_main_loop(); }
void OS_PS5::delete_main_loop() {
    if (main_loop) memdelete(main_loop);
    main_loop = nullptr;
}
void OS_PS5::set_main_loop(MainLoop *loop) { main_loop = loop; }
MainLoop *OS_PS5::get_main_loop() const { return main_loop; }
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
        if (DisplayServer::get_singleton())
            DisplayServer::get_singleton()->process_events();
        if (Main::iteration()) break;
    }
    main_loop->finalize();
}
