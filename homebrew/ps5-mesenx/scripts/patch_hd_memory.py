#!/usr/bin/env python3
"""PS5 MesenCE: bounded PNG decoding, lazy HD atlases and safe tile bounds.

The fixes are applied to a *copied* MesenCE-Libretro checkout, never to
tracked upstream files. Fail fast if source contracts change.
"""
from pathlib import Path
import sys

root=Path(sys.argv[1])
def edit(path,old,new):
    p=root/path
    src=p.read_text()
    n=src.count(old)
    if n!=1: raise RuntimeError(f"{path}: patch point count {n}: {old[:110]!r}")
    p.write_text(src.replace(old,new,1))

edit("Utilities/PNGHelper.h",
     "ReadPNG(vector<uint8_t> input, vector<T>& output",
     "ReadPNG(const vector<uint8_t>& input, vector<T>& output")
p=root/"Utilities/PNGHelper.cpp"
s=p.read_text()
if s.count("ReadPNG(vector<uint8_t> input, vector<T>& output")!=1: raise RuntimeError("PNG ReadPNG signature changed")
s=s.replace("ReadPNG(vector<uint8_t> input, vector<T>& output",
            "ReadPNG(const vector<uint8_t>& input, vector<T>& output")
for typ in ("uint8_t","uint32_t"):
    old=f"ReadPNG<{typ}>(vector<uint8_t> input,"
    if s.count(old)!=1:raise RuntimeError(f"missing PNG template {typ}")
    s=s.replace(old,f"ReadPNG<{typ}>(const vector<uint8_t>& input,")
old="""spng_ctx* ctx = spng_ctx_new(0);
	if((r = spng_set_crc_action"""
new="""// Never dereference an allocation failure from a large HD pack.
    spng_ctx* ctx = spng_ctx_new(0);
    if(!ctx) return -1;
	if((r = spng_set_crc_action"""
if s.count(old)!=1:raise RuntimeError("spng allocation anchor")
s=s.replace(old,new)
old="""image_width = ihdr.width;
	image_height = ihdr.height;

	int fmt = SPNG_FMT_RGBA8;"""
new="""// A compressed PNG may expand by hundreds of megabytes. Bound each
    // decoded image before spng allocates its output buffer.
    if(ihdr.width==0 || ihdr.height==0 || ihdr.width>4096 ||
       ihdr.height>4096 || uint64_t(ihdr.width)*ihdr.height>16777216ull) {
        spng_ctx_free(ctx);
        return -2;
    }
    image_width=ihdr.width;
    image_height=ihdr.height;

	int fmt = SPNG_FMT_RGBA8;"""
if s.count(old)!=1:raise RuntimeError("PNG validation anchor")
s=s.replace(old,new)
old="""out_image.resize(out_size / sizeof(T));"""
new="""if(out_size>64ull*1024*1024 || out_size%sizeof(T)!=0) {
        spng_ctx_free(ctx);
        return -3;
    }
    out_image.resize(out_size / sizeof(T));"""
if s.count(old)!=1:raise RuntimeError("PNG resize anchor")
s=s.replace(old,new)
p.write_text(s)

p=root/"Core/NES/HdPacks/HdData.h"
s=p.read_text()
if "#include <atomic>" not in s:s=s.replace("#pragma once","#pragma once\n#include <atomic>",1)
if s.count("uint32_t Width;\n\tuint32_t Height;\n\tbool IsFullyOpaque = false;\n\n\tvoid Init()")!=1:
    raise RuntimeError("MesenCE-Libretro HdPackBitmapInfo declaration changed")
s=s.replace("uint32_t Width;\n\tuint32_t Height;\n\tbool IsFullyOpaque = false;\n\n\tvoid Init()",
"""uint32_t Width=0;
    uint32_t Height=0;
    bool IsFullyOpaque = false;
    size_t _reservedBytes=0;
    inline static std::atomic<size_t> _totalBytes{0};
    ~HdPackBitmapInfo() {
        if(_reservedBytes)_totalBytes.fetch_sub(_reservedBytes,std::memory_order_relaxed);
    }

	void Init()""",1)
start=s.index("\t\t//Timer tmr;")
end=s.index("\n\t\t_initDone = true;",start)
s=s[:start]+"""        // PNG IHDR: limit per-atlas and total decoded pixels. Reserve
        // the budget BEFORE calling the decoder; release on invalid files.
        size_t bytes=0;
        if(FileData.size()>=24 &&
           std::memcmp(FileData.data(),"\\x89PNG\\r\\n\\x1a\\n",8)==0) {
            auto be32=[&](size_t at)->uint32_t {
                return (uint32_t(FileData[at])<<24)|(uint32_t(FileData[at+1])<<16)|
                       (uint32_t(FileData[at+2])<<8)|uint32_t(FileData[at+3]);
            };
            const uint32_t w=be32(16), h=be32(20);
            if(w && h && w<=4096 && h<=4096 && uint64_t(w)*h<=16777216ull)
                bytes=size_t(w)*h*4;
        }
        constexpr size_t kAtlasBudget=256ull*1024*1024;
        size_t current=_totalBytes.load(std::memory_order_relaxed);
        bool admitted=bytes>0;
        while(admitted) {
            if(bytes>kAtlasBudget || current>kAtlasBudget-bytes) {
                admitted=false;
                break;
            }
            if(_totalBytes.compare_exchange_weak(current,current+bytes,
                    std::memory_order_relaxed))break;
        }
        if(admitted) {
            _reservedBytes=bytes;
            bool decoded=false;
            try {decoded=PNGHelper::ReadPNG(FileData,PixelData,Width,Height);}
            catch(const std::bad_alloc&) {decoded=false;}
            if(!decoded || PixelData.size()!=bytes/4) {
                PixelData.clear();Width=0;Height=0;
                _totalBytes.fetch_sub(bytes,std::memory_order_relaxed);
                _reservedBytes=0;
                MessageManager::Log("[HDPack][PS5] Atlas decode denied/failed: "+PngName);
            } else {
                PremultiplyAlpha();
            }
        } else {
            Width=0;Height=0;PixelData.clear();
            MessageManager::Log("[HDPack][PS5] Atlas memory/dimension limit: "+PngName);
        }
        // Reclaim compressed image bytes as soon as decoding completes.
        vector<uint8_t>().swap(FileData);""" + s[end:]
# avoid empty tile data indexing 
needle="\t\tfor(size_t i = 0; i < HdTileData.size(); i++) {"
if s.count(needle)!=1:raise RuntimeError("tile flags anchor")
s=s.replace(needle, """        if(HdTileData.empty()) {
            Blank=true;HasTransparentPixels=true;IsFullyTransparent=true;
            return;
        }
"""+needle,1)
start=s.index("\t__noinline void Init()",s.index("struct HdPackTileInfo"))
end=s.index("\n\tstring ToString",start)
s=s[:start]+"""    __noinline void Init()
    {
        if(!_needInit)return;
        auto tile_lock=_initLock.AcquireSafe();
        if(!_needInit)return;
        _needInit=false;
        // An invalid, oversized or truncated PNG is NOT a valid tile.
        // Previous code resized and read HdTileData[0] even when empty.
        if(!Bitmap || !Width || !Height || Width>80 || Height>80)return;
        Bitmap->Init();
        if(!Bitmap->Width || !Bitmap->Height ||
           X>Bitmap->Width || Y>Bitmap->Height ||
           Width>Bitmap->Width-X || Height>Bitmap->Height-Y)return;
        const uint64_t end=(uint64_t(Y)+Height-1)*Bitmap->Width+X+Width;
        if(end>Bitmap->PixelData.size())return;
        try {
            HdTileData.resize(size_t(Width)*Height);
        } catch(const std::bad_alloc&) {
            return;
        }
        uint64_t offset=uint64_t(Y)*Bitmap->Width+X;
        for(uint32_t y=0;y<Height;++y) {
            memcpy(HdTileData.data()+size_t(y)*Width,
                   Bitmap->PixelData.data()+offset,size_t(Width)*sizeof(uint32_t));
            offset+=Bitmap->Width;
        }
        UpdateFlags();
    }
""" + s[end:]
start=s.index("    void PreloadResources()") if "    void PreloadResources()" in s else s.index("\tvoid PreloadResources()")
end=s.index("\tvoid CancelLoad()",start)
s=s[:start]+"""    void PreloadResources()
    {
        // One decoder at a time avoids transient 2x spikes from concurrent
        // PNG buffers. Preload at most 64 MiB, defer other atlases until
        // their first tile is requested by the emulation worker.
        Timer tmr;
        size_t budget=64ull*1024*1024,decoded=0,deferred=0;
        auto prewarm=[&](const vector<unique_ptr<HdPackBitmapInfo>>& images) {
            for(auto& img:images) {
                if(_cancelLoad)return;
                auto& d=img->FileData;
                if(d.size()<24 ||
                   std::memcmp(d.data(),"\\x89PNG\\r\\n\\x1a\\n",8)!=0) {
                    ++deferred;continue;
                }
                auto be32=[&](size_t at)->uint32_t {
                    return (uint32_t(d[at])<<24)|(uint32_t(d[at+1])<<16)|
                           (uint32_t(d[at+2])<<8)|uint32_t(d[at+3]);
                };
                const uint64_t pixels=uint64_t(be32(16))*be32(20);
                if(!pixels || pixels>16777216ull ||
                   pixels*4>budget-decoded) {++deferred;continue;}
                img->Init();
                decoded+=img->PixelData.size()*4;
            }
        };
        prewarm(BackgroundFileData);
        prewarm(ImageFileData);
        MessageManager::Log("[HDPack][PS5] preload complete bytes="+
            std::to_string(decoded)+" deferred="+std::to_string(deferred)+
            " tiles="+std::to_string(Tiles.size())+
            " ms="+std::to_string(tmr.GetElapsedMS()));
    }

""" + s[end:]
p.write_text(s)

p=root/"Core/NES/HdPacks/HdNesPack.cpp"
s=p.read_text()
needle="""				return hdPackTile;
"""
if s.count(needle)!=1:raise RuntimeError("HD matching tile return anchor")
s=s.replace(needle, """                // Refuse invalid/uninitialized HD tiles; fall back to
                // original NES pixels instead of dereferencing null data.
                if(!hdPackTile->HdTileData.empty())return hdPackTile;
""",1)
p.write_text(s)
print("PASS PS5 HD resource guard: 256MiB atlases, <=64MiB each, 64MiB prewarm, safe lazy tiles")


# PS5 HD first-frame crash guard / precise stage markers.
# These patches do NOT disable successful HD packs. They reject malformed
# frame/tile pointers that previously reached unchecked native memory accesses.
edit("Core/NES/HdPacks/HdVideoFilter.cpp",
     "#include \"NES/HdPacks/HdNesPack.h\"",
     "#include \"NES/HdPacks/HdNesPack.h\"\n#include <cstdio>\n#include <atomic>")

edit("Core/NES/HdPacks/HdVideoFilter.cpp",
     """\tuint32_t hdScale = _hdNesPack->GetScale();

\treturn {""",
     """\t// Some malformed packs advertise an unsupported scale. Do not
    // dereference a missing HdNesPack instantiated by the switch above.
    uint32_t hdScale = _hdNesPack ? _hdNesPack->GetScale() : 1;
    static std::atomic<unsigned> probes{0};
    if(probes.fetch_add(1,std::memory_order_relaxed)<4) {
        std::fprintf(stderr,"HDPACK_STAGE frame_info scale=%u renderer=%p\\n",
                     hdScale,(void*)_hdNesPack.get());
        std::fflush(stderr);
    }

\treturn {""")

edit("Core/NES/HdPacks/HdVideoFilter.cpp",
     """\tOverscanDimensions overscan = GetOverscan();
\t_hdNesPack->Process((HdScreenInfo*)_frameData, GetOutputBuffer(), overscan);""",
     """    uint32_t* output=GetOutputBuffer();
    if(!_hdNesPack || !output) {
        std::fprintf(stderr,"HDPACK_GUARD renderer_or_output_null pack=%p pixels=%p\\n",
                     (void*)_hdNesPack.get(),(void*)output);
        std::fflush(stderr);
        return;
    }
    OverscanDimensions overscan = GetOverscan();
    if(overscan.Left >= 256 || overscan.Right >= 256 ||
       overscan.Top >= 240 || overscan.Bottom >= 240 ||
       uint32_t(overscan.Left)+overscan.Right>=256 ||
       uint32_t(overscan.Top)+overscan.Bottom>=240) {
        std::fprintf(stderr,"HDPACK_GUARD invalid_overscan\\n");
        std::fflush(stderr);
        return;
    }
    static std::atomic<unsigned> hdFrames{0};
    unsigned frame=hdFrames.fetch_add(1,std::memory_order_relaxed)+1;
    if(frame<=4) {
        std::fprintf(stderr,"HDPACK_STAGE filter_begin frame=%u output=%p data=%p\\n",
                     frame,(void*)output,(void*)_frameData);
        std::fflush(stderr);
    }
    _hdNesPack->Process((HdScreenInfo*)_frameData, output, overscan);
    if(frame<=4) {
        std::fprintf(stderr,"HDPACK_STAGE filter_end frame=%u\\n",frame);
        std::fflush(stderr);
    }""")

edit("Core/NES/HdPacks/HdNesPack.cpp",
     """\tuint32_t* bitmapData = hdPackTileInfo.HdTileData.data();
\tuint32_t tileWidth = 8 * scale;""",
     """    // A malformed/over-budget atlas must never be sampled as a 64-pixel
    // tile. Lazy loading can produce an empty vector when PNG is rejected.
    if(tileInfo.OffsetX>=8 || tileInfo.OffsetY>=8 ||
       hdPackTileInfo.HdTileData.size()<size_t(64)*scale*scale)
        return;
    uint32_t* bitmapData=hdPackTileInfo.HdTileData.data();
    uint32_t tileWidth = 8 * scale;""")

edit("Core/NES/HdPacks/HdNesPack.cpp",
     """\tconst HdBgConfig& bgConfig = _bgConfig[(int)priority];
\tif((int32_t)x >= bgConfig.BgMinX""",
     """    const HdBgConfig& bgConfig = _bgConfig[(int)priority];
    if(bgConfig.Background && (!bgConfig.BgPixels ||
       bgConfig.BgPngWidth<=0 || bgConfig.BgSourceHeightHd<=0 ||
       bgConfig.BgSourceWidthHd<=0)) return;
    if((int32_t)x >= bgConfig.BgMinX""")

edit("Core/NES/HdPacks/HdNesPack.cpp",
     """\t_hdScreenInfo = hdScreenInfo;

\t// HD GAMEPLAY FAST-FORWARD CONTINUITY FIX V111F1""",
     """    if(!hdScreenInfo || !outputBuffer)return;
    static std::atomic<unsigned> packFrameCount{0};
    unsigned packFrame=packFrameCount.fetch_add(1,std::memory_order_relaxed)+1;
    if(packFrame<=4) {
        std::fprintf(stderr,"HDPACK_STAGE pack_process_begin frame=%u scale=%u\\n",
                     packFrame,unsigned(scale));
        std::fflush(stderr);
    }
    _hdScreenInfo = hdScreenInfo;

\t// HD GAMEPLAY FAST-FORWARD CONTINUITY FIX V111F1""")
# pch.h includes std::atomic but declare explicit in HdNesPack.cpp
edit("Core/NES/HdPacks/HdNesPack.cpp",
     "#include \"pch.h\"",
     "#include \"pch.h\"\n#include <cstdio>\n#include <atomic>")
print("PASS first-frame HD null/bounds guards and staged crash tracing")
