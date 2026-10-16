#!/usr/bin/env python3
"""PS5MesenX: keep all Libretro calls on one emulation worker thread."""
from pathlib import Path
import sys
root=Path(__file__).resolve().parents[1]
source=Path(sys.argv[1]); options=Path(sys.argv[2])
runtime=source/"shelf_runtime.cpp"; header=source/"shelf_runtime.h"
o=options.read_text()
needle='Toggle("NES/HdPacks", "HD Packs", "false", "", "Load Mesen hires textures from HdPacks/<ROM name>/hires.txt. Restart the game to apply.", true)'
assert o.count(needle)==1
o=o.replace(needle,
    'Toggle("NES/SafeMode", "Safe mode", "true", "", "Disable HD textures for smoother gameplay. Restart game to apply.", true), '+needle)
options.write_text(o)
r=runtime.read_text()
assert r.count("ShelfRuntime::ShelfRuntime(")==1
r=r[:r.index("ShelfRuntime::ShelfRuntime(")]
r=r.replace("#include <cstring>", "#include <cstring>\n#include <chrono>\n#include <vector>\n#include <map>")
r+=(root/"vendor/threaded_runtime.inc").read_text()
runtime.write_text(r)
h=header.read_text()
h=h.replace("#include <memory>","#include <memory>\n#include <atomic>\n#include <mutex>\n#include <thread>\n#include <vector>")
h=h.replace("    bool Active()const{return active;}",
    "    bool Active()const{return active;}\n    bool Loading()const{return active&&!worker_ready.load(std::memory_order_acquire);}")
h=h.replace("    Session core;", """    void RunWorker(std::string path,bool hd_enabled,std::vector<uint8_t> rom);
    std::thread worker;
    std::atomic<bool> request_stop{false},worker_done{true},worker_ready{false},worker_failed{false};
    std::atomic<uint16_t> pending_buttons{0};
    std::atomic<unsigned> actions{0};
    std::mutex snapshot_mutex;
    std::vector<uint32_t> snapshot_pixels;
    unsigned snapshot_width=256,snapshot_height=240,shown_width=256,shown_height=240;
    uint64_t produced=0,consumed=0;
    std::string worker_error;""")
h=h.replace("    std::unique_ptr<ShelfAudio> audio;","")
h=h.replace("    double previous=0,accumulated=0,last_save=0;","")
header.write_text(h)
assert "Session core;" not in h and "WORKER_READY" in r
assert "game_audio->Push" in r and "request_stop.store" in r
assert "GAME_READ_MAIN_BEGIN" in r and "GAME_READ_MAIN_OK" in r
assert r.index('GAME_READ_MAIN_BEGIN') < r.index('worker=std::thread(')
assert "RunWorker(path,hd_enabled,std::move(rom))" in r
worker_code=r.split("void ShelfRuntime::RunWorker(",1)[1]
assert "ReadRom(" not in worker_code, "ROM file access must happen before worker launch"
print("PASS: MesenCE worker owns init/load/run/unload/deinit; PS5 pad and Vulkan stay on UI thread")
