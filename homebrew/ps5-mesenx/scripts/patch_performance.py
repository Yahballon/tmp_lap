#!/usr/bin/env python3
"""Patch the native MesenX game runtime for controlled resource usage.

All MesenCE Libretro operations remain on the SAME frontend thread.
Never reintroduce the old cross-thread retro_load_game bug.
"""
from pathlib import Path
import sys
source=Path(sys.argv[1])
options_file=Path(sys.argv[2])
runtime=source/"shelf_runtime.cpp"
header=source/"shelf_runtime.h"

def once(text,old,new):
    assert text.count(old)==1, (old[:120],text.count(old))
    return text.replace(old,new,1)

o=options_file.read_text()
o=once(o,
    'Toggle("NES/HdPacks", "HD Packs", "false", "", "Load Mesen hires textures from HdPacks/<ROM name>/hires.txt. Restart the game to apply.", true)',
    'Toggle("NES/SafeMode", "Safe mode", "true", "", "Recommended: suppress HD packs and bound emulation work so the PS5 stays responsive. Restart the game to apply.", true), Toggle("NES/HdPacks", "HD Packs", "false", "", "HD textures can be expensive. Turn Safe mode Off before enabling. Restart the game.", true)')
options_file.write_text(o)

h=header.read_text()
h=once(h,'    bool active=false,sound=true;', '''    bool active=false,sound=true,safe_mode=true;
    unsigned perf_frame_counter=0,perf_slow_count=0;''')
header.write_text(h)

r=runtime.read_text()
r=once(r,'#include <cstring>', '#include <cstring>\n#include <chrono>')
r=once(r,'''    const bool hd_on=(hd_choice=="true"||hd_choice=="enabled");
    core.SetOption("mesen_hdpacks",hd_on?"enabled":"disabled");''',
'''    safe_mode=get("NES/SafeMode","true")!="false";
    const bool requested_hd=(hd_choice=="true"||hd_choice=="enabled");
    const bool hd_on=!safe_mode && requested_hd;
    mesen_diag::write("PERF_CONFIG safe=%d hd_requested=%d hd_enabled=%d",
                      int(safe_mode),int(requested_hd),int(hd_on));
    core.SetOption("mesen_hdpacks",hd_on?"enabled":"disabled");''')
r=once(r,'    mesen_diag::write("GAME_CORE_LOAD_BEGIN");', '''    mesen_diag::write("GAME_CORE_LOAD_BEGIN");
    const auto load_started=std::chrono::steady_clock::now();''')
r=once(r,'''    mesen_diag::write("GAME_CORE_LOAD_OK fps=%.3f sample_rate=%.0f",core.fps,core.sample_rate);''',
'''    mesen_diag::write("GAME_CORE_LOAD_OK fps=%.3f sample_rate=%.0f",core.fps,core.sample_rate);
    mesen_diag::write("PERF_LOAD_DURATION_MS %lld",static_cast<long long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now()-load_started).count()));''')
r=once(r,'menu_atlas=renderer.Atlas();active=true;previous=0;accumulated=0;last_save=0;',
'''menu_atlas=renderer.Atlas();active=true;previous=0;accumulated=0;last_save=0;
    perf_frame_counter=0;perf_slow_count=0;
    mesen_diag::write("PERF_GAME_READY safe=%d resolution=%ux%u",
                      int(safe_mode),core.width,core.height);''')
r=once(r,'''    while(accumulated>=1/core.fps&&runs++<6){core.Run();if(audio)audio->Push(resampler.Process(core.audio));accumulated-=1/core.fps;changed=true;}''',
'''    // A six-frame catch-up loop can stall the PS5 compositor and OS button.
    // Process just one frame in Safe Mode, two otherwise. Drop stale backlog.
    const double period=1.0/core.fps;
    const unsigned max_runs=safe_mode?1u:2u;
    while(accumulated>=period&&runs++<max_runs) {
        const auto began=std::chrono::steady_clock::now();
        core.Run();
        const auto core_ms=std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now()-began).count();
        if(audio)audio->Push(resampler.Process(core.audio));
        ++perf_frame_counter;
        if(core_ms>35 && (perf_slow_count<10 || perf_frame_counter%120==0)) {
            ++perf_slow_count;
            mesen_diag::write("PERF_FRAME_SLOW frame=%u core_ms=%lld size=%ux%u",
                perf_frame_counter,static_cast<long long>(core_ms),core.width,core.height);
        }
        accumulated-=period;changed=true;
    }
    if(accumulated>period) {
        // Catching up forever on an expensive HD frame only compounds lag.
        if(perf_frame_counter%300==0)
            mesen_diag::write("PERF_FRAME_BACKLOG_DROP frame=%u",perf_frame_counter);
        accumulated=0;
    }''')
r=once(r,'''    if(now-last_save>=5){if(!core.SaveBattery())error=core.error;last_save=now;}''',
'''    // Disk I/O every five seconds can interrupt input / Vulkan. Save on
    // stop and every 60s as a compromise. Time slow writes for diagnosis.
    if(now-last_save>=60){
        const auto began=std::chrono::steady_clock::now();
        if(!core.SaveBattery())error=core.error;
        const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now()-began).count();
        if(ms>50)mesen_diag::write("PERF_SRAM_SAVE_MS %lld",static_cast<long long>(ms));
        last_save=now;
    }''')
r=once(r,
    '''        auto* next=renderer.CreateTexture(core.width,core.height,VK_FORMAT_R8G8B8A8_UNORM,core.pixels.data());''',
'''        const auto upload_started=std::chrono::steady_clock::now();
        auto* next=renderer.CreateTexture(core.width,core.height,VK_FORMAT_R8G8B8A8_UNORM,core.pixels.data());
        const auto upload_ms=std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now()-upload_started).count();
        if(upload_ms>35 && (perf_slow_count<10 || perf_frame_counter%120==0))
            mesen_diag::write("PERF_UPLOAD_SLOW frame=%u upload_ms=%lld",
                perf_frame_counter,static_cast<long long>(upload_ms));''')
assert 'while(accumulated>=1/core.fps&&runs++<6)' not in r
runtime.write_text(r)
print("PASS: MesenX Safe Mode, HD isolation, bounded catchup, timing diagnostics")
