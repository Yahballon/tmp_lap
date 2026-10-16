#!/usr/bin/env python3
"""PS5MesenX native frontend hotfix for HD pack responsiveness.
Run on the already prepared frontend, not on the pinned upstream source.
"""
from pathlib import Path
import sys

source=Path(sys.argv[1])
header=source/"shelf_runtime.h"
runtime=source/"shelf_runtime.cpp"
def once(text, old, new):
    n=text.count(old)
    if n!=1: raise RuntimeError(f"expected 1 source patch point, got {n}: {old[:90]!r}")
    return text.replace(old,new,1)

h=header.read_text()
h=once(h,'#include <memory>', '#include <memory>\n#include <atomic>\n#include <thread>')
h=once(h,'    bool active=false,sound=true;', '''    std::thread load_thread;
    std::atomic<bool> load_done{false};
    bool loading=false,load_ok=false;
    std::string load_error;
    double last_loading_log=0;
    bool active=false,sound=true;''')
header.write_text(h)

r=runtime.read_text()
r=once(r,'#include <cstring>','#include <cstring>\n#include <chrono>\n#include <exception>')
start=r.index('ShelfRuntime::~ShelfRuntime()=default;')
end=r.index('void ShelfRuntime::Stop(',start)
r=r[:start]+'''ShelfRuntime::~ShelfRuntime() {
    if(load_thread.joinable())load_thread.join();
}
bool ShelfRuntime::Start(const std::string& path,fe::Renderer& renderer) {
    if(active || loading) {error="Another game is still loading";return false;}
    if(load_thread.joinable())load_thread.join();
    error.clear();
    load_error.clear();
    std::map<std::string,std::string> options;
    auto read=[&](const std::string& file){std::string text;fe::settings::ReadFile(file,text);
        for(const auto& item:fe::settings::ReadState(text).kv)options[item.first]=item.second;};
    read(root+"/gs.ini");
    std::string file=path.substr(path.find_last_of('/')+1);
    const auto dot=file.find_last_of('.');
    if(dot!=std::string::npos)file.resize(dot);
    read(root+"/settings/"+file+".ini");
    auto get=[&](const std::string& key,const std::string& fallback){auto at=options.find(key);return at==options.end()?fallback:at->second;};
    const std::string hd_choice=get("NES/HdPacks","false");
    const bool hd_on=(hd_choice=="true"||hd_choice=="enabled");
    core.SetOption("mesen_hdpacks",hd_on?"enabled":"disabled");
    mesen_diag::write("HDPACK_SETTING game=%s ui=%s core=%s",
        path.c_str(),hd_choice.c_str(),hd_on?"enabled":"disabled");
    aspect=get("NES/Aspect","4:3");bindings.Load(options);
    sound=get("NES/Sound","true")=="true";
    // Never run ZIP I/O, PNG decoding or HdPackLoader on the Vulkan/pad thread.
    // The menu frame continues to present while the core loads in this worker.
    loading=true;active=true;load_ok=false;load_done.store(false,std::memory_order_relaxed);
    last_loading_log=0;
    // The native PS5 frontend is built -fno-exceptions: propagate ordinary
    // load failures via booleans and error strings, never C++ try/catch.
    load_thread=std::thread([this,path]{
        const auto began=std::chrono::steady_clock::now();
        mesen_diag::write("HDPACK_LOAD_BEGIN game=%s",path.c_str());
        std::vector<uint8_t> rom;
        std::string read_error;
        load_ok=ReadRom(path,rom,read_error);
        if(load_ok)load_ok=core.Load(path,std::move(rom));
        if(!load_ok)load_error=read_error.empty()?core.error:read_error;
        const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now()-began).count();
        mesen_diag::write("HDPACK_LOAD_END ok=%d duration_ms=%lld error=%s",
            int(load_ok),static_cast<long long>(elapsed),load_error.c_str());
        load_done.store(true,std::memory_order_release);
    });
    return true;
}
''' +r[end:]
r=once(r,'bool ShelfRuntime::Build(fe::Renderer& renderer,fe::FrameDesc& frame,uint32_t raw,double now) {\n',
'''bool ShelfRuntime::Build(fe::Renderer& renderer,fe::FrameDesc& frame,uint32_t raw,double now) {
    if(loading) {
        // Render/poll gamepad on every tick even while HD pack PNGs are decoded.
        // Preserve the last menu frame instead of entering the emulation loop.
        if(!load_done.load(std::memory_order_acquire)) {
            if(now-last_loading_log>=5.0) {
                mesen_diag::write("HDPACK_LOAD_WAIT input_and_vulkan_loop_alive=1");
                last_loading_log=now;
            }
            return true;
        }
        if(load_thread.joinable())load_thread.join();
        loading=false;
        if(!load_ok) { error=load_error;active=false;return false; }
        resampler.Reset(core.sample_rate);
        if(sound) {
            audio=std::make_unique<ShelfAudio>();
            if(!audio->Open()){error="MesenCE audio port failed";audio.reset();}
        }
        menu_atlas=renderer.Atlas();
        previous=0;accumulated=0;last_save=0;
        mesen_diag::write("HDPACK_LOAD_READY game_input_enabled=1");
        return true;
    }
''')
runtime.write_text(r)
assert 'HDPACK_LOAD_BEGIN' in r and 'load_done.load(std::memory_order_acquire)' in r
print("PASS: async PS5 game/HD pack loader and live pad/Vulkan pump")
