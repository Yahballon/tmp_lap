// Minimal GPL-compatible MesenCE smoke test: original synthetic NROM data.
// No commercial ROM, game art, BIOS or HD pack is bundled here.
#include "core_session.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

int main() {
    char temp[] = "/tmp/ps5-mesenx-smoke-XXXXXX";
    char* base = mkdtemp(temp);
    assert(base != nullptr);
    const std::string root(base);
    std::vector<uint8_t> rom(16 + 0x4000 + 0x2000, 0);
    std::memcpy(rom.data(), "NES\x1A", 4);
    rom[4] = 1; // 16 KiB PRG-ROM, mapper 0
    rom[5] = 1; // 8 KiB CHR-ROM
    rom[16] = 0x4C; // JMP $8000, a legal self-contained 6502 loop
    rom[17] = 0x00;
    rom[18] = 0x80;
    for (size_t at = 16 + 0x3FFA; at <= 16 + 0x3FFE; at += 2) {
        rom[at] = 0x00;
        rom[at + 1] = 0x80;
    }

    snes::Session session(root + "/saves");
    if (!session.Load("Synthetic.nes", std::move(rom))) {
        std::fprintf(stderr, "MesenCE synthetic ROM load FAILED: %s\n",
                     session.error.c_str());
        return 1;
    }
    session.buttons[0] = 1u << 8; // NES A
    unsigned frames = 0;
    for (int i = 0; i < 130; ++i) {
        session.Run();
        if (!session.pixels.empty()) {
            assert(session.width && session.height);
            assert(session.pixels.size() == size_t(session.width) * session.height);
            ++frames;
        }
        if (frames >= 3) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
    if (frames == 0) {
        std::fprintf(stderr, "MesenCE video bridge did not yield any frames\n");
        return 2;
    }
    if (!session.SaveState()) {
        std::fprintf(stderr, "MesenCE save state FAILED: %s\n", session.error.c_str());
        return 3;
    }
    if (!session.LoadState()) {
        std::fprintf(stderr, "MesenCE load state FAILED: %s\n", session.error.c_str());
        return 4;
    }
    session.Unload();
    std::printf("PASS: synthetic NES loaded, %u video frames, input, state roundtrip\n", frames);
    return 0;
}
