#!/usr/bin/env python3
"""Make a strictly scoped *build-configuration* patch to pinned ZQuest CMake.

The vanilla ZQuest CMake config forcibly enables stduuid OS-specific
libuuid (unavailable on PS5). stduuid itself supports a portable mode.
This PS5-only diagnostic disables the OS UUID generator without changing
uuid APIs, quest parsing, ZASM instructions, or engine behavior.
Desktop native file dialog is disabled through WANT_NFD=OFF in CMake args.
"""
import argparse
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument("cmake_root",type=Path)
a=p.parse_args()
cmake=a.cmake_root/"CMakeLists.txt"
text=cmake.read_text()
old='set(UUID_SYSTEM_GENERATOR ON CACHE INTERNAL "")'
new='set(UUID_SYSTEM_GENERATOR OFF CACHE INTERNAL "")'
assert text.count(old)==1,"Upstream UUID configuration changed; re-evaluate"
assert 'target_link_libraries(zplayer PRIVATE stduuid)' in text
assert 'FetchContent_MakeAvailable(stduuid)' in text
pulse='if(NOT EMSCRIPTEN AND LINUX AND NOT "pulseaudio.c" IN_LIST ALLEGRO_AUDIO_SOURCES)'
assert text.count(pulse)==1,"Pinned upstream PulseAudio CI guard changed"
pulse_probe='if(NOT EMSCRIPTEN AND LINUX AND NOT ZC_PS5_AUDIO_DRIVER_PROBE AND NOT "pulseaudio.c" IN_LIST ALLEGRO_AUDIO_SOURCES)'
text=text.replace(pulse,pulse_probe,1)
# PS5 native app is linked into one ELF; Allegro shared .so packages do
# not match the PS5-DevBridge FSELF distribution contract.
shared='set(SHARED ON CACHE BOOL "" FORCE)'
assert text.count(shared)==1,"Pinned upstream Allegro shared setting changed"
text=text.replace(shared,'set(SHARED OFF CACHE BOOL "" FORCE)',1)
packaging="add_subdirectory(packaging)"
assert text.count(packaging)==1,"Upstream packaging entry changed"
text=text.replace(packaging,'if(NOT ZC_PS5_TARGET_ONLY_PROBE)\n  add_subdirectory(packaging)\nendif()',1)
cmake.write_text(text.replace(old,new,1))

# Three Allegro-Legacy file format registries use Windows stricmp().
# The PS5 SDK implements POSIX strcasecmp() with identical behavior here.
# Patch staging-only sources and pin expected counts for safe upgrades.
for filename, expected in (("readbmp.c",2),("readfont.c",1),("readsmp.c",2)):
    srcpath=a.cmake_root/"third_party/allegro_legacy/src"/filename
    src=srcpath.read_text()
    assert src.count("stricmp(")==expected, (
        f"Upstream {filename} casefold implementation changed"
    )
    assert "#include <string.h>" in src and "#include <strings.h>" not in src
    src=src.replace("#include <string.h>","#include <string.h>\\n#include <strings.h>",1)
    src=src.replace("stricmp(","strcasecmp(")
    srcpath.write_text(src)
