#!/usr/bin/env python3
"""Cross-compile genuine MesenCE-Libretro for PS5. No synthetic eboot."""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, as_completed
import os, subprocess
root=Path(__file__).resolve().parents[1]
src=root/"build/MesenCE-Libretro"
sdk=Path(os.environ["PS5_PAYLOAD_SDK"])
wrapper=root/"upstream/PS5SNESX/ps5/proto/native/tooling/prospero-clang18"
out=root/"build/ps5-core"
assert sdk.joinpath("target/include").exists() and wrapper.is_file()
cpp=[src/"Libretro/libretro.cpp",src/"Core/pch.cpp"]
for part in ("Core/NES","Core/Shared","Core/Netplay","Utilities"):
    cpp+=list((src/part).rglob("*.cpp"))
cpp+=[p for p in (src/"Core/Debugger").rglob("*.cpp") if not p.name.startswith("ExpressionEvaluator.") or p.name=="ExpressionEvaluator.cpp"]
cpp+=[src/"Core/Debugger/ExpressionEvaluator.Nes.cpp"]
c=list((src/"SevenZip").rglob("*.c"))+list((src/"Utilities").rglob("*.c"))
c += [p for p in (src/"Lua").rglob("*.c") if p.name not in {"serial.c","unix.c","unixdgram.c","unixstream.c"}]
# Actual rendered-video fallback: the frame decoder can complete while
# VideoRenderer::GetLastFrame() is still empty on a headless PS5.
video_h = src/"Core/Shared/Video/VideoDecoder.h"
video_cpp = src/"Core/Shared/Video/VideoDecoder.cpp"
libretro = src/"Libretro/libretro.cpp"
h = video_h.read_text()
needle = "\tuint32_t GetFrameCount();"
assert h.count(needle)==1
h = h.replace(needle, "\tuint32_t* GetDecodedOutputBuffer();\\n"+needle)
h = h.replace("\\n", "\n")
video_h.write_text(h)
s = video_cpp.read_text()
needle = "uint32_t VideoDecoder::GetFrameCount()"
assert s.count(needle)==1
s = s.replace(needle, """uint32_t* VideoDecoder::GetDecodedOutputBuffer()
{
    // Called only after WaitForAsyncFrameDecode() and before the next
    // RunFrame(). The filter owns this decoded BGRA32 framebuffer.
    return _videoFilter ? _videoFilter->GetOutputBuffer() : nullptr;
}

"""+needle)
video_cpp.write_text(s)
s = libretro.read_text()
# libretro.cpp advertises Core Options V2 exclusively.  Our native frontend
# intentionally implements the compact Libretro environment, so
# RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2 is not accepted and mesen_palette
# remains unset. Upstream then fills NesConfig.UserPalette with BLACK for
# every NES color (0xff000000); audio works but ALL games render black.
# Supply the genuine built-in Mesen 64-color default when no palette was
# received. Respect user choices whenever a palette was actually supplied.
palette_point = "\t\tif(_userRgbPalette.size() >= 512) {"
assert s.count(palette_point)==1
assert "uint32_t defaultPalette[0x40]" in s
s=s.replace(palette_point, """\t\tif(_userRgbPalette.empty()) {
            _userRgbPalette.assign(std::begin(defaultPalette), std::end(defaultPalette));
            if(logCallback)
                logCallback(RETRO_LOG_INFO,
                    "[MesenCE][PS5Palette] Missing Core Options V2 palette: installed default 64-color NES palette\\n");
        }
""" + palette_point)
assert "Missing Core Options V2 palette" in s
needle = "\t\tbool validFrame = frame.FrameBuffer && frame.Width > 0 && frame.Height > 0;"
assert s.count(needle)==1
s=s.replace(needle, """\t\t// PS5 headless renderer compatibility: if the intermediate
        // VideoRenderer has not published a frame yet, use the completed
        // VideoDecoder's actual RGB buffer directly. Never invent a frame.
        if(!frame.FrameBuffer && decoder && !frontendFastForward) {
            FrameInfo decoded = decoder->GetFrameInfo();
            uint32_t* decodedBuffer = decoder->GetDecodedOutputBuffer();
            if(decodedBuffer && decoded.Width > 0 && decoded.Height > 0 &&
               decoded.Width <= 2560 && decoded.Height <= 2560 &&
               decoder->GetLastDecodedFrameNumber() > 0) {
                frame.FrameBuffer=decodedBuffer;
                frame.Width=decoded.Width;
                frame.Height=decoded.Height;
                frame.FrameNumber=decoder->GetLastDecodedFrameNumber();
                if(logCallback && (_libretroRunCounter<=5 || (_libretroRunCounter%300)==0))
                    logCallback(RETRO_LOG_INFO,
                        "[MesenCE][PS5Video] direct decoded fallback %ux%u frame=%u\\n",
                        frame.Width,frame.Height,frame.FrameNumber);
            }
        }
"""+needle)
# Force explicit user choice at the actual ROM load boundary, not merely
# at the earlier init options phase. The core normally defaults to enabled
# when SET_CORE_OPTIONS_V2 is unsupported by our native Libretro host.
setting='\t\t_hdPacksEnabled = enabled(MesenHdPacks, true);'
assert setting in s
s=s.replace(setting,'\t\t_hdPacksEnabled = enabled(MesenHdPacks, false);',1)
explicit='\t\t\tNesConfig nesCfg = _emu->GetSettings()->GetNesConfig();\n\t\t\tnesCfg.EnableHdPacks = _hdPacksEnabled;'
assert s.count(explicit)==1
s=s.replace(explicit,'''            retro_variable hd_choice{MesenHdPacks, nullptr};
            const bool hd_allowed =
                env_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &hd_choice) &&
                hd_choice.value && std::strcmp(hd_choice.value,"enabled")==0;
            _hdPacksEnabled=hd_allowed;
            if(logCallback) logCallback(RETRO_LOG_INFO,
                "[MesenCE][HDPack] ROM loading: requested=%s enabled=%d\\\\n",
                hd_choice.value?hd_choice.value:"unset",int(hd_allowed));
            NesConfig nesCfg = _emu->GetSettings()->GetNesConfig();
            nesCfg.EnableHdPacks = hd_allowed;'''.replace('\\\\n','\\n'),1)

libretro.write_text(s)

# PS5 HD pack initialization used to decode every PNG and premultiply all
# tile pixels on the same core-load thread. Divide independent image/tile
# initialization across two CPU workers; preserve the existing Init locks.
hd_data=src/"Core/NES/HdPacks/HdData.h"
hd=hd_data.read_text()
assert "void PreloadResources()" in hd and "void CancelLoad()" in hd
hd=hd.replace("#pragma once", "#pragma once\n#include <thread>\n#include <atomic>", 1)
a=hd.index("\tvoid PreloadResources()")
b=hd.index("\tvoid CancelLoad()",a)
hd=hd[:a]+"""    void PreloadResources()
    {
        Timer tmr;
        // Decode independently on two workers. All workers join BEFORE game
        // execution, so no bitmap/tile references can escape initialization.
        auto parallel=[&](size_t count, auto work) {
            std::atomic<size_t> index{0};
            auto worker=[&]{
                for(;;) {
                    const size_t i=index.fetch_add(1,std::memory_order_relaxed);
                    if(i>=count)break;
                    if(_cancelLoad)break;
                    work(i);
                }
            };
            std::thread second(worker);
            worker();
            second.join();
        };
        parallel(BackgroundFileData.size(), [&](size_t i) {
            if(BackgroundFileData[i])BackgroundFileData[i]->Init();
        });
        if(_cancelLoad)return;
        parallel(ImageFileData.size(), [&](size_t i) {
            if(ImageFileData[i])ImageFileData[i]->Init();
        });
        if(_cancelLoad)return;
        // HD tile pixels are a duplicate of the already-decoded PNG atlas.
        // Init() allocates a fresh Width*Height RGBA buffer for EACH tile.
        // For large Zelda packs that duplicates entire texture sheets and
        // increases working-set pressure. HdNesPack::GetMatchingTile()
        // already calls tile->Init() on first actual use, so preserve lazy
        // initialization and do not expand all unused tiles at ROM load.
        MessageManager::Log("[HDPack] PS5 atlas preload complete: images=" +
            std::to_string(ImageFileData.size()) + " deferredTiles=" +
            std::to_string(Tiles.size()) + " elapsedMs=" +
            std::to_string(tmr.GetElapsedMS()));
    }

""" + hd[b:]
hd_data.write_text(hd)

sources=sorted(set(c+cpp))
assert len(sources)>100 and all(p.is_file() for p in sources)
# Large Mesen HD packs spend most CPU time in software tile blending and
# conditional video filtering. -O1 was used throughout the initial bring-up;
# optimize the hot files aggressively, keeping conservative flags elsewhere.
common=["-O2","-fPIC","-march=znver2","-ffunction-sections","-fdata-sections","-DLIBRETRO"]
common += ["-I"+str(src/p) for p in (".","Core","Utilities","Libretro","Core/Shared","Core/NES")]
def build(p):
    obj=out/p.relative_to(src).with_suffix(".o")
    obj.parent.mkdir(parents=True,exist_ok=True)
    if p.suffix==".cpp":
        # The HD renderer is purely CPU-based; the PS5 Vulkan frontend only
        # uploads the completed frame. Prioritize inner-loop optimization.
        hot=(p.parent.name=="HdPacks" and p.name in
             {"HdNesPack.cpp","HdVideoFilter.cpp","HdNesPpu.cpp"}) or (
                 p.name in {"VideoDecoder.cpp","libretro.cpp"})
        flags=[("-O3" if x=="-O2" else x) for x in common] if hot else common
        cmd=["sh",str(wrapper),*flags,"-std=c++17","-fexceptions","-frtti","-Wno-undefined-inline"]
    else:
        cmd=["clang-18","-target","x86_64-sie-ps5","-isysroot",str(sdk),
             "-isystem",str(sdk/"target/include"),*common,"-std=gnu11"]
    run=subprocess.run([*cmd,"-c",str(p),"-o",str(obj)],capture_output=True,text=True)
    return p,obj,run.returncode,run.stderr[-3200:]
print("Cross-building",len(sources),"native MesenCE objects",flush=True)
failure=0
with ThreadPoolExecutor(max_workers=4) as ex:
    jobs=[ex.submit(build,p) for p in sources]
    for task in as_completed(jobs):
        p,obj,status,errors=task.result()
        if status:
            failure+=1
            print("FAILED",p.relative_to(src),errors,flush=True)
            if failure>=8:break
if failure:raise SystemExit(f"{failure} PS5 core object(s) failed to compile")
archive=out/"libmesen_ce_nes.ps5.a"
subprocess.run(["llvm-ar-18","rcs",str(archive),*(str(out/p.relative_to(src).with_suffix(".o")) for p in sources)],check=True)
assert archive.stat().st_size>100_000
print("PASS real PS5 MesenCE archive:",archive,flush=True)
