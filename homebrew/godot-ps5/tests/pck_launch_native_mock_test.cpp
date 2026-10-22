// SPDX-License-Identifier: MIT
#define GODOT_PS5_NATIVE_IO 1
#include "../engine/platform/ps5/pck_launch_config.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

static std::string selected_config;
static std::string expected_external;
static bool config_present = true;
static int opens = 0;
extern "C" int sceKernelOpen(const char *path, int, int) {
    ++opens;
    const std::string name(path);
    if (name == "/data/GodotPS5/launch.txt") return config_present ? 10 : -1;
    if (name == expected_external && !expected_external.empty()) return 11;
    if (name == "/app0/assets/godot/smoke.pck") return 12;
    if (name == "/data/GodotPS5/games" || name.find("/godot") != std::string::npos)
        return 13; // Mock empty game directories
    return -1;
}
extern "C" long sceKernelRead(int fd, void *buf, size_t len) {
    if (fd == 10) {
        const size_t count = std::min(len, selected_config.size());
        std::memcpy(buf, selected_config.data(), count);
        return long(count);
    }
    if (fd == 11 || fd == 12) {
        const unsigned char h[20] = {
            'G','D','P','C',2,0,0,0,4,0,0,0,7,0,0,0,0,0,0,0
        };
        const size_t count = std::min(len, sizeof(h));
        std::memcpy(buf, h, count);
        return long(count);
    }
    return -1;
}
extern "C" int sceKernelGetdents(int, char *, int) { return 0; }
extern "C" int sceKernelClose(int) { return 0; }

int main() {
    using namespace godot_ps5;
    selected_config = "/mnt/usb0/godot/External.PCK\n";
    expected_external = "/mnt/usb0/godot/External.PCK";
    auto x = select_runtime_pack();
    assert(x.source == PackSource::Config && x.path == expected_external);
    selected_config = "/mnt/usb0/godot/../INVALID.pck";
    x = select_runtime_pack();
    assert(x.source == PackSource::BundledSmoke);
    assert(x.path == "/app0/assets/godot/smoke.pck");
    config_present = false;
    x = select_runtime_pack();
    assert(x.source == PackSource::BundledSmoke);
    assert(opens > 3);
    std::cout << "PASS: PS5 libkernel config / external PCK / bundled fallback\n";
}
