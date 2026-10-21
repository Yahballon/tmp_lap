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
# Keep every vendored C/C++ dependency static for the single-ELF PS5 build.
# In particular libgme.so cannot be linked as a standalone shared object
# against the PS5 SDK's unbundled libc and libc++ runtime.
shared_default="if(EMSCRIPTEN)\n\tset(BUILD_SHARED_LIBS OFF)\nelse()\n\tset(BUILD_SHARED_LIBS ON)\nendif()"
assert text.count(shared_default)==1,"Upstream library linkage policy changed"
text=text.replace(shared_default,"if(EMSCRIPTEN OR ZC_PS5_TARGET_ONLY_PROBE)\n\tset(BUILD_SHARED_LIBS OFF)\nelse()\n\tset(BUILD_SHARED_LIBS ON)\nendif()",1)
# Allegro's desktop-native file-dialog addon is optional and not present in
# the PS5 backend. Preserve actual symbols: any runtime dependence on it
# must still fail the final link rather than silently providing stubs.
dialog='if (NOT EMSCRIPTEN)\n\ttarget_link_libraries(allegro_with_legacy INTERFACE allegro_dialog)\nendif()'
assert text.count(dialog)==1, "Pinned Allegro dialog linkage changed"
text=text.replace(dialog, 'if (NOT EMSCRIPTEN AND NOT ZC_PS5_TARGET_ONLY_PROBE)\n\ttarget_link_libraries(allegro_with_legacy INTERFACE allegro_dialog)\nendif()', 1)

cmake.write_text(text.replace(old,new,1))

# zcsound normally becomes a desktop .so, but the native PS5 package is a
# single homebrew ELF. Use the same original static library sources as the
# upstream Emscripten configuration rather than changing audio behavior.
sound=a.cmake_root/"src/components/sound/CMakeLists.txt"
sound_text=sound.read_text()
sound_old='if(EMSCRIPTEN)\n\tadd_library(zcsound STATIC ${ZCSOUNDSOURCES})'
assert sound_text.count(sound_old)==1, "Pinned zcsound linkage changed"
sound.write_text(sound_text.replace(sound_old,
    'if(EMSCRIPTEN OR ZC_PS5_TARGET_ONLY_PROBE)\n\tadd_library(zcsound STATIC ${ZCSOUNDSOURCES})', 1))


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
    src=src.replace("#include <string.h>","#include <string.h>\n#include <strings.h>",1)
    src=src.replace("stricmp(","strcasecmp(")
    srcpath.write_text(src)

# Upstream SDL MIDI device implementation is explicitly an unimplemented
# no-output fallback. It includes SDL_mixer.h but never uses Mix_* APIs.
# Omit only its unused include so the missing SDK header doesn't block
# unrelated original zplayer compilation. MIDI remains NOT IMPLEMENTED.
midi=a.cmake_root/"third_party/allegro_legacy/src/a5/midia5/sdl/sdl_midi.c"
source=midi.read_text()
assert source.count("#include <SDL2/SDL_mixer.h>")==1
assert "// also, it's not implemented" in source
assert "Mix_" not in source
source=source.replace("#include <SDL2/SDL_mixer.h>","/* PS5 probe: unused SDL_mixer include removed; MIDI unsupported */",1)
midi.write_text(source)


# Clang's PS5 libc defines NULL as nullptr, while upstream macro ERR_EXIT
# returns NULL from launch_process(), whose return type is process_killer.
# Fix only this incorrect return-site, preserving spawn-error logging and
# the defined empty process_killer state (pid=0). Do not change macro calls
# used by pointer-returning functions elsewhere.
proc=a.cmake_root/"src/base/process_management.cpp"
source=proc.read_text()
site='if (s != 0) ERR_EXIT("Failed posix_spawnp", (process_manager*)0, 0);'
assert source.count(site)==1,"Original process_killer error handling changed"
assert "process_killer launch_process(" in source
fix=r'''if (s != 0) {
        zprint2("[PROCESS_LAUNCH: '%s' ERROR %d]: %s\n",
                file.c_str(), s, "Failed posix_spawnp");
        return process_killer();
    }'''
source=source.replace(site,fix,1)
proc.write_text(source)


# Desktop-only foreground switching has nothing to do on a native PS5;
# mark its parameter deliberately unused. Retain all desktop branches.
zalleg=a.cmake_root/"src/zalleg/zalleg.cpp"
source=zalleg.read_text()
needle="void zalleg_bring_window_to_foreground(ALLEGRO_DISPLAY* display)\n{\n"
assert source.count(needle)==1,"Pinned upstream zalleg foreground signature changed"
source=source.replace(needle,needle+"\t(void)display; // PS5: no desktop window foreground API\n",1)
zalleg.write_text(source)


# The original cross-platform in-game file picker uses Windows stricmp for
# matching quest extensions. POSIX strcasecmp is the semantic equivalent.
chooser=a.cmake_root/"src/core/jwinfsel.cpp"
source=chooser.read_text()
assert source.count("stricmp(ext, tok)")==1,"Original file filter lookup changed"
assert '#include <cstring>' in source and '#include <strings.h>' not in source
source=source.replace('#include <cstring>','#include <cstring>\n#include <strings.h>',1)
source=source.replace('stricmp(ext, tok)','strcasecmp(ext, tok)',1)
chooser.write_text(source)


# Cross-compile actual zplayer script VM's case-insensitive comparisons.
# ZScript behavior remains ASCII/POSIX casefolding like legacy stricmp.
ffscript=a.cmake_root/"src/zc/ffscript.cpp"
source=ffscript.read_text()
assert source.count("stricmp(strA.c_str(), strB.c_str())")==3
assert '#include <string>' in source and '#include <strings.h>' not in source
source=source.replace('#include <string>','#include <string>\n#include <strings.h>',1)
source=source.replace('stricmp(strA.c_str(), strB.c_str())',
                      'strcasecmp(strA.c_str(), strB.c_str())')
ffscript.write_text(source)

# ZQuest's message intro spaces comparison also uses legacy Windows stricmp.
messages=a.cmake_root/"src/zc/message_string.cpp"
source=messages.read_text()
assert source.count("stricmp(")==1
assert '#include "zc/message_string.h"' in source
source=source.replace('#include "zc/message_string.h"',
                      '#include <strings.h>\n#include "zc/message_string.h"',1)
source=source.replace("stricmp(","strcasecmp(",1)
messages.write_text(source)

# Portable UUID generation for replay metadata: the original system generator
# requires libuuid, which the PS5 SDK does not supply. The upstream stduuid
# fork supports uuid_random_generator driven by standard mt19937. Do not stub
# UUIDs or reuse a fixed ID. PS5 random_device runtime needs hardware testing.
zelda=a.cmake_root/"src/zc/zelda.cpp"
source=zelda.read_text()
assert source.count("uuids::uuid_system_generator{}()")==2
assert '#include <random>' not in source
assert '#include <memory>' in source
source=source.replace('#include <memory>','#include <memory>\n#include <random>',1)
source=source.replace('auto uuid = uuids::uuid_system_generator{}();',
    'std::mt19937 rng(std::random_device{}());\n\t\t\tauto uuid = uuids::uuid_random_generator{rng}();')
zelda.write_text(source)

# The desktop window-raise helper has no body on the PS5/SDL target.
# Annotate its sole parameter, retaining the function and behavior.
zalleg=a.cmake_root/"src/zalleg/zalleg.cpp"
zs=zalleg.read_text()
signature="void zalleg_bring_window_to_foreground(ALLEGRO_DISPLAY* display)"
assert zs.count(signature)==1,"Upstream zalleg signature changed"
zs=zs.replace(signature,"void zalleg_bring_window_to_foreground([[maybe_unused]] ALLEGRO_DISPLAY* display)",1)
zalleg.write_text(zs)

# PS5-DevBridge's working ELF linker imports native SDK functions from the
# real target/lib/*.so SDK stub libraries. CMake's generic -lScePad etc.
# names are not SDK files. Link actual stubs and retain strict undefined-
# symbol checking; do not replace PS5 OS interfaces with dummy symbols.
root_cmake=a.cmake_root/"CMakeLists.txt"
root_text=root_cmake.read_text()
assert "add_executable(zplayer " in root_text
assert "target_link_libraries(zplayer PRIVATE core zcsound" in root_text
root_text += r'''
if(ZC_PS5_TARGET_ONLY_PROBE)
  file(GLOB ZC_PS5_IMPORT_STUBS "$ENV{PS5_PAYLOAD_SDK}/target/lib/*.so")
  if(NOT ZC_PS5_IMPORT_STUBS)
    message(FATAL_ERROR "PS5 SDK import stubs not found")
  endif()
  list(LENGTH ZC_PS5_IMPORT_STUBS _zc_ps5_stub_count)
  message(STATUS "PS5 SDK stub imports for original zplayer: ${_zc_ps5_stub_count}")
  target_link_libraries(zplayer PRIVATE ${ZC_PS5_IMPORT_STUBS})
  get_target_property(_zc_link zplayer LINK_LIBRARIES)
  message(STATUS "PS5 zplayer link interface: ${_zc_link}")
endif()
'''
root_cmake.write_text(root_text)
