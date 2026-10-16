// (AI-assisted) SNES core callbacks and persistent saves. SPDX-License-Identifier: GPL-3.0-or-later
#include "core_session.h"
#include "fe_file_io.h"
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <sys/stat.h>
#include <string>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include "rom_diagnostics.h"

namespace snes {
Session* Session::current = nullptr;
Session::Session(std::string directory) : m_directory(std::move(directory)) {
    m_system_dir = m_directory;
    if(m_system_dir.size()>=6 && m_system_dir.substr(m_system_dir.size()-6)=="/saves")
        m_system_dir.resize(m_system_dir.size()-6);
    current = this;
    // MesenCE emits FrameProbe counters once per core run and every 300 frames.
    setenv("MESENCE_FRAME_DIAGNOSTICS", "1", 1);
    mesen_diag::write("VIDEO_DIAG enabled=MESENCE_FRAME_DIAGNOSTICS");
    retro_set_environment(Environment);
    retro_set_video_refresh(Video);
    retro_set_audio_sample(Sample);
    retro_set_audio_sample_batch(Samples);
    retro_set_input_poll([]{});
    retro_set_input_state(Input);
    retro_init();
    retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
    retro_set_controller_port_device(1, RETRO_DEVICE_JOYPAD);
}
Session::~Session() { Unload(); retro_deinit(); current = nullptr; }
void Session::Log(enum retro_log_level level, const char* format, ...) {
    if (!format) return;
    char message[1536];
    va_list args;
    va_start(args, format);
    const int n=std::vsnprintf(message,sizeof(message),format,args);
    va_end(args);
    if(n<0)return;
    std::printf("[mesence] %s",message);
    mesen_diag::write("MESENCE_CORE level=%u %s",unsigned(level),message);
}
bool Session::Environment(unsigned cmd, void* data) {
    auto& s = *current;
    switch (cmd) {
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        static_cast<retro_log_callback*>(data)->log = Log; return true;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
        *static_cast<const char**>(data) = s.m_system_dir.c_str(); return true;
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *static_cast<const char**>(data) = s.m_directory.c_str(); return true;
    case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
        *static_cast<unsigned*>(data) = 0; return true;
    case RETRO_ENVIRONMENT_SET_VARIABLES:
        for (auto* v = static_cast<retro_variable*>(data); v && v->key; ++v) {
            if (!v->value) continue;
            std::string text = v->value;
            const auto semi = text.find(';');
            if (semi == std::string::npos) continue;
            const auto start = text.find_first_not_of(' ', semi + 1);
            if (start != std::string::npos)
                s.m_variables.try_emplace(v->key, text.substr(start, text.find('|', start) - start));
        }
        s.m_variables.try_emplace("mesen_hdpacks","enabled");
        return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE: {
        auto* v = static_cast<retro_variable*>(data);
        auto found = s.m_variables.find(v->key ? v->key : "");
        v->value = found == s.m_variables.end() ? nullptr : found->second.c_str();
        if(v->key && std::strcmp(v->key,"mesen_hdpacks")==0)
            mesen_diag::write("HDPACK_CORE_QUERY value=%s",v->value?v->value:"not-set");
        return v->value != nullptr;
    }
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        *static_cast<bool*>(data) = false; return true;
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
        const auto format = *static_cast<retro_pixel_format*>(data);
        if (format != RETRO_PIXEL_FORMAT_XRGB8888 && format != RETRO_PIXEL_FORMAT_RGB565 &&
            format != RETRO_PIXEL_FORMAT_0RGB1555) return false;
        s.m_format = format; return true;
    }
    case RETRO_ENVIRONMENT_GET_GAME_INFO_EXT:
        *static_cast<const retro_game_info_ext**>(data) = &s.m_info; return true;
    case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS: return true;
    case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        *static_cast<bool*>(data) = false; return true;
    case RETRO_ENVIRONMENT_GET_LANGUAGE:
        *static_cast<unsigned*>(data) = RETRO_LANGUAGE_ENGLISH; return true;
    case RETRO_ENVIRONMENT_SET_GEOMETRY: return true;
    case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO: {
        const auto& av = *static_cast<retro_system_av_info*>(data);
        s.fps = av.timing.fps; s.sample_rate = av.timing.sample_rate; return true;
    }
    case RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE:
    case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
    case RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS:
    case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL: return true;
    default: return false;
    }
}
bool Session::Load(const std::string& path, std::vector<uint8_t> rom) {
    if (!Unload()) return false;
    error.clear();
    if (rom.size() < 16 || rom.size() > (32u << 20)) { error = "Invalid ROM size"; return false; }
    m_path = path; m_rom = std::move(rom);
    const auto slash = path.find_last_of('/');
    m_content_dir = slash == std::string::npos ? "" : path.substr(0, slash);
    m_name = path.substr(slash == std::string::npos ? 0 : slash + 1);
    const auto dot = m_name.find_last_of('.');
    m_extension = dot == std::string::npos ? "nes" : m_name.substr(dot + 1);
    if (dot != std::string::npos) m_name.resize(dot);
    // Content-derived names avoid collisions for similarly named games on different shares.
    uint64_t hash = UINT64_C(14695981039346656037);
    for (uint8_t byte : m_rom) { hash ^= byte; hash *= UINT64_C(1099511628211); }
    char key[32]; std::snprintf(key, sizeof(key), "%016llx", static_cast<unsigned long long>(hash));
    m_save_path = m_directory + "/" + key;
    m_info = {};
    m_info.full_path = m_path.c_str(); m_info.dir = m_content_dir.c_str();
    m_info.name = m_name.c_str(); m_info.ext = m_extension.c_str();
    m_info.data = m_rom.data(); m_info.size = m_rom.size(); m_info.persistent_data = true;
    // MesenCE needs an accessible ROM path to match HdPacks/<ROM stem>/hires.txt.
    for (const std::string& dir : {m_system_dir, m_system_dir+"/cache", m_system_dir+"/cache/roms", m_directory}) {
        if (mkdir(dir.c_str(),0777)!=0 && errno!=EEXIST) {
            error="Unable to create NES cache directory"; return false;
        }
    }
    // MesenCE searches the system-directory HdPacks/<ROM basename>/hires.txt.
    const std::string hd_value = m_variables.count("mesen_hdpacks")
        ? m_variables.at("mesen_hdpacks") : "disabled";
    const std::string hires=m_system_dir+"/HdPacks/"+m_name+"/hires.txt";
    const int pack_fd=open(hires.c_str(),O_RDONLY);
    const bool exists=pack_fd>=0;
    const int hd_errno=exists?0:errno;
    if(exists)close(pack_fd);
    mesen_diag::write("HDPACK_PROBE game=%s setting=%s file=%s found=%d errno=%d",
                     m_name.c_str(),hd_value.c_str(),hires.c_str(),int(exists),hd_errno);
    m_path = m_system_dir+"/cache/roms/"+m_name+".nes";
    if (!fe::fileio::WriteAtomic(m_path, std::string(reinterpret_cast<const char*>(m_rom.data()),m_rom.size()), error)) return false;
    m_extension="nes";
    m_info.full_path=m_path.c_str();m_info.ext=m_extension.c_str();
    retro_deinit();retro_init(); // Apply MesenCE options before ROM loading.
    mesen_diag::write("HDPACK_APPLY game=%s option=%s",m_name.c_str(),hd_value.c_str());
    retro_game_info info{m_path.c_str(), m_rom.data(), m_rom.size(), nullptr};
    if (!retro_load_game(&info)) { error = "MesenCE core could not load this NES ROM"; m_rom.clear(); return false; }
    m_loaded = true;
    retro_system_av_info av{}; retro_get_system_av_info(&av);
    fps = av.timing.fps; sample_rate = av.timing.sample_rate;
    if (fps < 40 || fps > 75 || sample_rate < 8000 || sample_rate > 192000) {
        error = "Unsupported core timing or audio rate"; retro_unload_game(); m_loaded = false; return false;
    }
    std::string saved, read_error;
    if (!fe::fileio::Read(m_save_path + ".sav", saved, read_error, true)) error = read_error;
    const size_t size = retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
    void* memory = retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    if (memory && saved.size() == size && size) std::memcpy(memory, saved.data(), size);
    else if (!saved.empty()) error = "Battery save size does not match this ROM";
    std::printf("[mesenx] loaded %s: %.3f fps, %.0f Hz, SRAM %zu\n", path.c_str(), fps, sample_rate, size);
    return true;
}
bool Session::SaveBattery() {
    if (!m_loaded) return true;
    const size_t size = retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
    const auto* memory = static_cast<const char*>(retro_get_memory_data(RETRO_MEMORY_SAVE_RAM));
    if (!memory || !size) return true;
    return fe::fileio::WriteAtomic(m_save_path + ".sav", std::string(memory, size), error);
}
bool Session::Unload() {
    if (m_loaded) {
        if (!SaveBattery()) return false;
        retro_unload_game(); m_loaded = false;
    }
    m_rom.clear(); pixels.clear(); audio.clear(); buttons[0] = buttons[1] = 0;
    return true;
}
bool Session::SaveState() {
    if (!m_loaded) return false;
    std::string bytes(retro_serialize_size(), '\0');
    if (bytes.empty() || !retro_serialize(bytes.data(), bytes.size())) { error = "Save state failed"; return false; }
    return fe::fileio::WriteAtomic(m_save_path + ".state", bytes, error);
}
bool Session::LoadState() {
    if (!m_loaded) return false;
    std::string bytes;
    if (!fe::fileio::Read(m_save_path + ".state", bytes, error)) return false;
    if (bytes.size() != retro_serialize_size() || !retro_unserialize(bytes.data(), bytes.size())) {
        error = "Incompatible save state"; return false;
    }
    return true;
}
void Session::Run() {
    if (!m_loaded) return;
    audio.clear();
    retro_run();
    ++m_run_count;
    if (m_run_count == 1 || m_run_count == 120 || m_run_count % 300 == 0) {
        mesen_diag::write("VIDEO_STATUS runs=%u callbacks=%u null=%u pixels=%zu dim=%ux%u format=%d audio_samples=%zu",
            m_run_count,m_video_callbacks,m_null_video_callbacks,pixels.size(),
            width,height,int(m_format),audio.size());
    }
}
int16_t Session::Input(unsigned port, unsigned device, unsigned, unsigned id) {
    if (port >= 2 || device != RETRO_DEVICE_JOYPAD) return 0;
    if (id == RETRO_DEVICE_ID_JOYPAD_MASK) return static_cast<int16_t>(current->buttons[port]);
    return id < 16 && (current->buttons[port] & (1u << id)) ? 1 : 0;
}
void Session::Sample(int16_t left, int16_t right) { int16_t pair[]{left,right}; Samples(pair,1); }
size_t Session::Samples(const int16_t* data, size_t frames) {
    if (!data || frames > 8192 || current->audio.size() + frames * 2 > 32768) return 0;
    current->audio.insert(current->audio.end(), data, data + frames * 2); return frames;
}
void Session::Video(const void* data, unsigned w, unsigned h, size_t pitch) {
    auto& s = *current;
    if (!data) {
        ++s.m_null_video_callbacks;
        if(s.m_null_video_callbacks==1 || s.m_null_video_callbacks%300==0)
            mesen_diag::write("VIDEO_NULL_FRAME callbacks=%u",s.m_null_video_callbacks);
        return;
    }
    ++s.m_video_callbacks;
    if(s.m_video_callbacks<=3 || s.m_video_callbacks%300==0) {
        uint32_t first=0;
        std::memcpy(&first,data,s.m_format==RETRO_PIXEL_FORMAT_XRGB8888?4:2);
        mesen_diag::write("VIDEO_CALLBACK count=%u width=%u height=%u pitch=%zu fmt=%d pixel0=0x%08x",
            s.m_video_callbacks,w,h,pitch,int(s.m_format),first);
    }
    const size_t bytes = s.m_format == RETRO_PIXEL_FORMAT_XRGB8888 ? 4 : 2;
    if (!w || !h || w > 4096 || h > 4096 || size_t(w)*h>4096u*4096u || pitch < size_t(w)*bytes) { s.error = "Invalid MesenCE HD video frame"; return; }
    s.width = w; s.height = h; s.pixels.resize(w * h);
    for (unsigned y=0;y<h;++y) for (unsigned x=0;x<w;++x) {
        const auto* at = static_cast<const uint8_t*>(data) + y * pitch + x * bytes;
        uint32_t r,g,b;
        if (bytes == 4) { uint32_t p; std::memcpy(&p,at,4); r=(p>>16)&255;g=(p>>8)&255;b=p&255; }
        else { uint16_t p; std::memcpy(&p,at,2); bool rgb565=s.m_format==RETRO_PIXEL_FORMAT_RGB565;
            r=((p>>(rgb565?11:10))&31)*255/31;g=((p>>5)&(rgb565?63:31))*255/(rgb565?63:31);b=(p&31)*255/31; }
        s.pixels[y*w+x] = 0xff000000u | r | (g<<8) | (b<<16);
    }
    // Log a 16x16 grid, not just pixel (0,0) which is usually black
    // in normal NES titles. Distinguishes an all-black core output from
    // a Vulkan sampling / shader issue without exposing ROM contents.
    if (s.m_video_callbacks==120 || s.m_video_callbacks%300==0) {
        uint32_t nonblack=0, bright=0;
        uint32_t hash=2166136261u;
        for (unsigned sy=0;sy<16;++sy) for (unsigned sx=0;sx<16;++sx) {
            uint32_t pixel=s.pixels[((sy*h)/16)*w+((sx*w)/16)];
            hash=(hash^pixel)*16777619u;
            if ((pixel&0x00ffffffu)!=0) ++nonblack;
            if (((pixel&255u)+((pixel>>8)&255u)+((pixel>>16)&255u))>384u) ++bright;
        }
        mesen_diag::write("VIDEO_COLOR callbacks=%u nonblack_256=%u bright_256=%u hash=%08x center=%08x",
            s.m_video_callbacks,nonblack,bright,hash,s.pixels[(h/2)*w+w/2]);
    }
}
}
