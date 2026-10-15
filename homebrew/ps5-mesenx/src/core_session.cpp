// PS5MesenX MesenCE adapter (upstream: nesdev-org/MesenCE).
// SPDX-License-Identifier: GPL-3.0-or-later
#include "core_session.h"
#include "fe_file_io.h"
#include "Core/Shared/Emulator.h"
#include "Core/Shared/EmuSettings.h"
#include "Core/Shared/SaveStateManager.h"
#include "Core/Shared/BaseControlDevice.h"
#include "Core/Shared/Interfaces/IInputProvider.h"
#include "Core/Shared/Interfaces/IAudioDevice.h"
#include "Core/Shared/Interfaces/IRenderingDevice.h"
#include "Core/Shared/RenderedFrame.h"
#include "Core/NES/Input/NesController.h"
#include "Utilities/FolderUtilities.h"
#include <atomic>
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <limits>
#include <mutex>
#include <sys/stat.h>

namespace snes {
namespace {
bool EnsureDirectory(const std::string& path) {
    if (path.empty()) return false;
    if (mkdir(path.c_str(), 0777) == 0) return true;
    struct stat sb{};
    return errno == EEXIST && stat(path.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode);
}
std::string FileName(const std::string& path) {
    const auto pos = path.find_last_of("/\\");
    return path.substr(pos == std::string::npos ? 0 : pos + 1);
}
std::string Stem(const std::string& file) {
    const auto dot = file.rfind('.');
    return file.substr(0, dot == std::string::npos ? file.size() : dot);
}
} // namespace

struct Session::State final : IInputProvider {
    Emulator emu;
    std::atomic<uint16_t> pressed{0};
    std::mutex video_lock, audio_lock;
    std::vector<uint32_t> video_pixels;
    std::vector<int16_t> samples;
    unsigned frame_width = 0, frame_height = 0;
    std::atomic<bool> got_video{false};
    std::atomic<uint32_t> rate{48000};

    struct VideoDevice final : IRenderingDevice {
        State* owner;
        explicit VideoDevice(State* s) : owner(s) {}
        void UpdateFrame(RenderedFrame& frame) override {
            // HD packs may produce frames beyond the old SNES 512x576 limit.
            constexpr size_t max_pixels = 4096u * 4096u;
            if (!frame.FrameBuffer || !frame.Width || !frame.Height ||
                frame.Width > 4096 || frame.Height > 4096 ||
                size_t(frame.Width) * frame.Height > max_pixels) return;
            const auto* source = static_cast<const uint32_t*>(frame.FrameBuffer);
            const size_t count = size_t(frame.Width) * frame.Height;
            std::lock_guard<std::mutex> lock(owner->video_lock);
            owner->video_pixels.assign(source, source + count);
            owner->frame_width = frame.Width;
            owner->frame_height = frame.Height;
            owner->got_video.store(true, std::memory_order_release);
        }
        void ClearFrame() override {
            std::lock_guard<std::mutex> lock(owner->video_lock);
            owner->video_pixels.clear();
            owner->got_video.store(false, std::memory_order_release);
        }
        void Render(RenderSurfaceInfo&, RenderSurfaceInfo&) override {}
        void Reset() override { ClearFrame(); }
        void SetFullscreenMode(FullscreenSettings) override {}
    };

    struct AudioDevice final : IAudioDevice {
        State* owner;
        explicit AudioDevice(State* s) : owner(s) {}
        void PlayBuffer(int16_t* buffer, uint32_t frames,
                        uint32_t sample_rate, bool stereo) override {
            if (!buffer || frames > 16384) return;
            owner->rate.store(sample_rate, std::memory_order_relaxed);
            std::lock_guard<std::mutex> lock(owner->audio_lock);
            // Bound queue growth if the UI is paused or suspended.
            constexpr size_t limit = 48000 * 2;
            size_t amount = size_t(frames) * 2;
            if (owner->samples.size() + amount > limit) {
                owner->samples.clear();
            }
            if (stereo) {
                owner->samples.insert(owner->samples.end(), buffer, buffer + amount);
            } else {
                for (uint32_t i = 0; i < frames; ++i) {
                    owner->samples.push_back(buffer[i]);
                    owner->samples.push_back(buffer[i]);
                }
            }
        }
        void Stop() override {
            std::lock_guard<std::mutex> lock(owner->audio_lock);
            owner->samples.clear();
        }
        void Pause() override { Stop(); }
        void ProcessEndOfFrame() override {}
        string GetAvailableDevices() override { return "PS5 native bridge"; }
        void SetAudioDevice(string) override {}
        AudioStatistics GetStatistics() override { return {}; }
    };

    // Mesen asks input providers during the NES CPU's input polling.
    // The ShelfController ABI is libretro: A=8, B=0, Start=3,
    // Select=2, U/D/L/R=4/5/6/7. Map it to Mesen's own button enum.
    bool SetInput(BaseControlDevice* device) override {
        if (!device || device->GetPort() != 0) return false;
        const auto type = device->GetControllerType();
        if (type != ControllerType::NesController &&
            type != ControllerType::FamicomController &&
            type != ControllerType::FamicomControllerP2) return false;
        const uint16_t b = pressed.load(std::memory_order_relaxed);
        device->SetBitValue(NesController::A, (b & (1u << 8)) != 0);
        device->SetBitValue(NesController::B, (b & (1u << 0)) != 0);
        device->SetBitValue(NesController::Start, (b & (1u << 3)) != 0);
        device->SetBitValue(NesController::Select, (b & (1u << 2)) != 0);
        device->SetBitValue(NesController::Up, (b & (1u << 4)) != 0);
        device->SetBitValue(NesController::Down, (b & (1u << 5)) != 0);
        device->SetBitValue(NesController::Left, (b & (1u << 6)) != 0);
        device->SetBitValue(NesController::Right, (b & (1u << 7)) != 0);
        return true;
    }

    explicit State(bool hd_packs_enabled) {
        emu.Initialize(false);
        auto& nes = emu.GetSettings()->GetNesConfig();
        nes.EnableHdPacks = hd_packs_enabled;
        auto& audio = emu.GetSettings()->GetAudioConfig();
        audio.SampleRate = 48000;
        emu.SetAudioVideoInitCallback(
            [this]() -> IAudioDevice* { return new AudioDevice(this); },
            [this]() -> IRenderingDevice* { return new VideoDevice(this); });
    }
    ~State() {
        emu.Stop(false, true);
        emu.Release();
    }
};

Session::Session(std::string save_directory)
    : save_dir_(std::move(save_directory)) {
    // Frontend passed <root>/saves. MesenCE uses <root>/HdPacks and <root>/Saves.
    if (save_dir_.size() > 6 &&
        save_dir_.compare(save_dir_.size() - 6, 6, "/saves") == 0)
        root_ = save_dir_.substr(0, save_dir_.size() - 6);
    else
        root_ = save_dir_;
}

Session::~Session() { Unload(); }

void Session::SetOption(const std::string& key, const std::string& value) {
    if (key == "mesen_hd_packs") hd_enabled_ = value != "disabled";
}

bool Session::Load(const std::string& original_path, std::vector<uint8_t> rom) {
    if (!Unload()) return false;
    error.clear();
    if (rom.size() < 16 || rom.size() > (64u << 20)) {
        error = "Invalid NES ROM size";
        return false;
    }
    std::string file = FileName(original_path);
    if (file.empty() || Stem(file).empty() ||
        file.find("..") != std::string::npos ||
        file.find('/') != std::string::npos ||
        file.find('\\') != std::string::npos) {
        error = "Unsafe ROM filename";
        return false;
    }
    const std::string ext = file.substr(file.find_last_of('.') == std::string::npos ?
                                            file.size() : file.find_last_of('.'));
    if (ext != ".nes" && ext != ".NES") {
        error = "This PS5MesenX milestone supports NES ROMs only";
        return false;
    }

    if (!EnsureDirectory(root_) || !EnsureDirectory(root_ + "/cache") ||
        !EnsureDirectory(root_ + "/cache/roms") ||
        !EnsureDirectory(root_ + "/SaveStates") ||
        !EnsureDirectory(root_ + "/HdPacks") ||
        !EnsureDirectory(root_ + "/Saves")) {
        error = "Could not create PS5MesenX data folders";
        return false;
    }
    // Staged file keeps original ROM stem so Mesen finds HdPacks/<ROM stem>/hires.txt.
    rom_path_ = root_ + "/cache/roms/" + file;
    state_path_ = root_ + "/SaveStates/" + Stem(file) + ".mss";
    std::string payload(reinterpret_cast<const char*>(rom.data()), rom.size());
    if (!fe::fileio::WriteAtomic(rom_path_, payload, error)) return false;

    FolderUtilities::SetHomeFolder(root_);
    state_.reset(new State(hd_enabled_));
    try {
        if (!state_->emu.LoadRom(VirtualFile(rom_path_), VirtualFile())) {
            error = "MesenCE rejected the NES ROM (check ROM and HD pack revision)";
            state_.reset();
            return false;
        }
        state_->emu.RegisterInputProvider(state_.get());
        fps = state_->emu.GetFps();
        if (fps < 40 || fps > 75) fps = 60.0988;
        sample_rate = 48000;
        is_loaded_ = true;
        std::printf("[PS5MesenX] loaded NES %s, hdpack=%d, fps=%.3f\n",
                    file.c_str(), int(hd_enabled_), fps);
    } catch (const std::exception& e) {
        error = e.what();
        state_.reset();
        return false;
    }
    return true;
}

bool Session::Unload() {
    if (state_) {
        if (is_loaded_) state_->emu.UnregisterInputProvider(state_.get());
        state_.reset();
    }
    is_loaded_ = false;
    pixels.clear();
    audio.clear();
    width = 256; height = 240;
    buttons[0] = buttons[1] = 0;
    return true;
}

void Session::Run() {
    if (!is_loaded_ || !state_) return;
    state_->pressed.store(buttons[0], std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock(state_->video_lock);
        if (state_->got_video.exchange(false, std::memory_order_acq_rel)) {
            pixels.swap(state_->video_pixels);
            width = state_->frame_width;
            height = state_->frame_height;
        }
    }
    {
        std::lock_guard<std::mutex> lock(state_->audio_lock);
        audio.swap(state_->samples);
    }
    sample_rate = state_->rate.load(std::memory_order_relaxed);
}

bool Session::SaveBattery() {
    // MesenCE manages NES battery data via its own BatteryManager, including
    // save-on-stop. Never reinterpret this as bsnes SRAM.
    return true;
}
bool Session::SaveState() {
    if (!is_loaded_ || !state_) return false;
    if (!state_->emu.GetSaveStateManager()->SaveState(state_path_)) {
        error = "MesenCE could not save state";
        return false;
    }
    return true;
}
bool Session::LoadState() {
    if (!is_loaded_ || !state_) return false;
    if (!state_->emu.GetSaveStateManager()->LoadState(state_path_)) {
        error = "MesenCE save state missing or incompatible";
        return false;
    }
    return true;
}
} // namespace snes
