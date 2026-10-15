// PS5MesenX native MesenCE session bridge, GPL-3.0-or-later.
// This replaces the bsnes-mercury-only libretro Session. Never mix its saves
// or on-disk ROM cache with PS5SNESX.
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace snes {
class Session final {
public:
    explicit Session(std::string save_directory);
    ~Session();
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    bool Load(const std::string& path, std::vector<uint8_t> rom);
    bool Unload();
    void Run();
    bool SaveBattery();
    bool SaveState();
    bool LoadState();
    void SetOption(const std::string& key, const std::string& value);
    bool loaded() const { return is_loaded_; }

    std::string error;
    std::vector<uint32_t> pixels;
    std::vector<int16_t> audio;
    unsigned width = 256, height = 240;
    double fps = 60.0988, sample_rate = 48000;
    uint16_t buttons[2] = {};

private:
    struct State;
    std::unique_ptr<State> state_;
    std::string save_dir_, root_, rom_path_, state_path_;
    bool is_loaded_ = false;
    bool hd_enabled_ = true;
};
} // namespace snes
