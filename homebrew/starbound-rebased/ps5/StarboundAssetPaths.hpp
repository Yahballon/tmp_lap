#pragma once
// The PS5 libc/ShadowMount loader does not guarantee that chdir() selects
// the directory used by SDL_image, SDL_ttf, libstdc++ streams and SDL_mixer.
// Always resolve *every* upstream "res/..." path to a verified absolute root.
// This header is injected into source files only when they use game resources.
#include <cstdio>
#include <cstring>

namespace sb_resources {
inline void log(const char* tag, const char* value) {
#ifdef __FreeBSD__
    if (FILE* f = std::fopen("/data/starbound-rebased/boot.log", "a")) {
        std::fprintf(f, "%s %s\n", tag, value ? value : "");
        std::fflush(f);
        std::fclose(f);
    }
#else
    (void)tag; (void)value;
#endif
}
inline const char* root() {
    static const char* selected = []() -> const char* {
        // Prefer app0, but select only if an actual package file can be opened.
        // A successful chdir() alone is NOT proof of a working filesystem path.
        static const char* candidates[] = {
            "/app0/assets/",
            "/data/homebrew/PPSA99642/assets/",
            "/mnt/usb0/homebrew/PPSA99642/assets/",
            "/mnt/ext0/homebrew/PPSA99642/assets/",
            "/app0/",
            "/data/homebrew/PPSA99642/",
            "assets/",
            "./",
        };
        char file[512];
        for (const char* base : candidates) {
            std::snprintf(file, sizeof(file), "%sres/sprites/icons/logo.png", base);
            if (FILE* fp = std::fopen(file, "rb")) {
                std::fclose(fp);
                log("ASSET_ROOT_FOUND", base);
                return base;
            }
            log("ASSET_ROOT_MISS", file);
        }
        log("ASSET_ROOT_ERROR", "no readable logo in any candidate root");
        return "";
    }();
    return selected;
}

inline const char* hd_root() {
    // Only enable the alternate source when the entire HD pack is present.
    // An incomplete USB transfer must never replace working in-package art.
    static const char* selected=[]() -> const char* {
        static const char* candidates[] = {
            "/mnt/usb0/homebrew/starbound-rebased-hd/",
            "/mnt/ext0/homebrew/starbound-rebased-hd/",
            "/data/starbound-rebased/hd/",
            "/data/homebrew/PPSA99642/hd/",
        };
        static const char* required[] = {
            "res/sprites/characters/PlayerSpriteSheet.png",
            "res/sprites/characters/EnemySpriteSheet.png",
            "res/sprites/blocks/BlockTileSheet.png",
            "res/sprites/icons/logo.png",
            "res/sprites/backgrounds/alien_sky.png",
        };
        char filename[512];
        for (const char* base: candidates) {
            bool complete=true;
            for (const char* relative: required) {
                std::snprintf(filename,sizeof(filename),"%s%s",base,relative);
                FILE* fp=std::fopen(filename,"rb");
                if (!fp) { complete=false; break; }
                std::fclose(fp);
            }
            if (complete) {
                log("HD_ASSET_ROOT_FOUND",base);
                return base;
            }
        }
        log("HD_ASSET_ROOT_MISS","using built-in source art");
        return "";
    }();
    return selected;
}
inline bool hd_enabled() { return hd_root()[0] != '\0'; }

inline const char* path(const char* relative) {
    // Rotating buffers avoid lifetime issues for multiple arguments and allow
    // callers expecting const char*, std::string or std::ifstream filenames.
    static char slots[32][1024];
    static unsigned index = 0;
    const unsigned slot = (index++) % 32;
    // Since v0.1.14 all HD sprites are bundled IN the tested package.
    // Never let an outdated optional USB/HD art folder shadow the assets
    // built into PPSA99642. This makes upgrades reliable through TESTBUILDS.
    const char* base = root();
    std::snprintf(slots[slot], sizeof(slots[slot]), "%s%s", base, relative);
    return slots[slot];
}
} // namespace sb_resources
#define SB_ASSET_PATH(relative) sb_resources::path(relative)
