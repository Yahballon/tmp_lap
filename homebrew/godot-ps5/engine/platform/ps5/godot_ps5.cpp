// SPDX-License-Identifier: MIT
// Link this static-engine entry point from a genuine PS5-DevBridge launcher.
#include "os_ps5.h"
#include "pck_locator.hpp"
#include "core/profiling/profiling.h"
#include "main/main.h"
#include <cstdio>
#include <cstdlib>
#include <string>

extern "C" int godot_ps5_run_pack(const char *requested) {
    const std::string pack = godot_ps5::choose_pack(requested);
    if (pack.empty()) {
        std::fprintf(stderr, "[GodotPS5] No valid standalone Godot 4 pack found.\n");
        return EXIT_FAILURE;
    }
    std::fprintf(stderr, "[GodotPS5] Headless Godot startup: %s\n", pack.c_str());
    godot_init_profiler();
    int result = EXIT_FAILURE;
    {
        OS_PS5 os;
        char headless[] = "--headless";
        char main_pack[] = "--main-pack";
        char *args[] = {headless, main_pack, const_cast<char *>(pack.c_str())};
        const Error err = Main::setup("godot.ps5", 3, args);
        if (err == OK) {
            if (Main::start() == EXIT_SUCCESS) {
                os.run();
                result = os.get_exit_code();
            }
            Main::cleanup();
        } else {
            std::fprintf(stderr, "[GodotPS5] Engine setup error: %d\n", int(err));
        }
    }
    godot_cleanup_profiler();
    return result;
}
