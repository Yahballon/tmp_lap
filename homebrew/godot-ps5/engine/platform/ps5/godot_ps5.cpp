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

    std::fprintf(stderr, "[GodotPS5 PHASE 0.2.9] BEFORE_PROFILER_INIT\n");
    std::fflush(stderr);
    godot_init_profiler();
    std::fprintf(stderr, "[GodotPS5 PHASE 0.2.9] AFTER_PROFILER_INIT\n");
    std::fflush(stderr);
    int result = EXIT_FAILURE;
    {
        std::fprintf(stderr, "[GodotPS5 PHASE 0.2.9] BEFORE_OS_PS5_CONSTRUCTOR\n");
        std::fflush(stderr);
        OS_PS5 os;
        std::fprintf(stderr, "[GodotPS5 PHASE 0.2.9] AFTER_OS_PS5_CONSTRUCTOR\n");
        std::fflush(stderr);
        char headless[] = "--headless";
        char main_pack[] = "--main-pack";
        char *args[] = {headless, main_pack, const_cast<char *>(pack.c_str())};

        std::fprintf(stderr, "[GodotPS5 PHASE 0.2.9] BEFORE_MAIN_SETUP\n");
        std::fflush(stderr);
        const Error err = Main::setup("godot.ps5", 3, args);
        std::fprintf(stderr, "[GodotPS5 PHASE 0.2.9] AFTER_MAIN_SETUP status=%d\n", int(err));
        std::fflush(stderr);
        if (err == OK) {

            std::fprintf(stderr, "[GodotPS5 PHASE 0.2.9] BEFORE_MAIN_START\n");
            std::fflush(stderr);
            const int start_status = Main::start();
            std::fprintf(stderr, "[GodotPS5 PHASE 0.2.9] AFTER_MAIN_START status=%d\n", start_status);
            std::fflush(stderr);
            if (start_status == EXIT_SUCCESS) {
                std::fprintf(stderr, "[GodotPS5 PHASE 0.2.9] BEFORE_OS_RUN\n");
                std::fflush(stderr);
                os.run();
                result = os.get_exit_code();
            }
            Main::cleanup();
        } else {
            std::fprintf(stderr, "[GodotPS5] Engine setup error: %d\n", int(err));
        }
    }
    std::fprintf(stderr, "[GodotPS5 PHASE 0.2.9] BEFORE_PROFILER_CLEANUP\n");
    std::fflush(stderr);
    godot_cleanup_profiler();
    return result;
}
