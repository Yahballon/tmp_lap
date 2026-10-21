// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace godot_ps5 {
inline bool pack_name(std::string n) {
    if (n.size() < 5) return false;
    n = n.substr(n.size()-4);
    for (char &c : n) if (c >= 'A' && c <= 'Z') c = char(c + ('a'-'A'));
    return n == ".pck";
}
inline unsigned read_u32(const unsigned char *p) {
    return unsigned(p[0]) | unsigned(p[1]) << 8 |
           unsigned(p[2]) << 16 | unsigned(p[3]) << 24;
}
inline bool valid_godot4_pack(const std::string &path) {
    if (path.empty() || path.size() >= 1024 || path[0] != '/') return false;
    struct stat st {};
    if (lstat(path.c_str(), &st) || !S_ISREG(st.st_mode) || st.st_size < 20) return false;
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) return false;
    unsigned char header[20] {};
    const size_t n = fread(header, 1, sizeof(header), f);
    fclose(f);
    return n == sizeof(header) && std::memcmp(header,"GDPC",4)==0 &&
        read_u32(header+4) >= 2 && read_u32(header+4) <= 4 &&
        read_u32(header+8) == 4;
}
inline std::string first_pack_in(const char *root) {
    DIR *d = opendir(root);
    if (!d) return {};
    std::vector<std::string> found;
    size_t scanned = 0;
    while (dirent *e = readdir(d)) {
        if (++scanned > 1024) break;
        std::string n(e->d_name);
        if (n.empty() || n[0] == '.' || !pack_name(n)) continue;
        const std::string candidate = std::string(root) + "/" + n;
        if (valid_godot4_pack(candidate)) found.push_back(candidate);
    }
    closedir(d);
    std::sort(found.begin(), found.end());
    return found.empty() ? std::string() : found.front();
}
inline std::string choose_pack(const char *explicit_path = nullptr) {
    if (explicit_path && *explicit_path)
        return valid_godot4_pack(explicit_path) ? std::string(explicit_path) : std::string();
    for (const char *root : {"/data/GodotPS5/games", "/mnt/usb0/godot",
                              "/mnt/usb1/godot", "/mnt/usb0/GodotPS5/games"}) {
        const auto found = first_pack_in(root);
        if (!found.empty()) return found;
    }
    return {};
}
} // namespace godot_ps5
