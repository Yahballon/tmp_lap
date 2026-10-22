#!/usr/bin/env python3
"""Select the existing *real* PS5 SDL2 backend for a PS5-DevBridge crossbuild.

PS5 uses a FreeBSD-derived libc, but CMake's generic FreeBSD selection is
not the PS5 device/video/audio/joystick implementation. The pinned OpenAGC
SDL2 fork has a genuine elseif(PS5) backend, but its CMake platform detection
does not recognize the x86_64-sie-ps5/FreeBSD cross toolchain.
Only adjust SDL's platform detection in this staged checkout; no fake devices,
undefined symbol suppression, or stubbed audio/video functions.
"""
from pathlib import Path
import argparse

p = argparse.ArgumentParser()
p.add_argument("source", type=Path)
args = p.parse_args()

platform = args.source / "cmake/sdlplatform.cmake"
source = platform.read_text()
needle = "  if(WIN32)\n    set(SDL_CMAKE_PLATFORM WINDOWS)"
assert source.count(needle) == 1, "Pinned SDL2 platform detection changed"
assert "  if(SDL_CMAKE_PLATFORM)\n    set(${SDL_CMAKE_PLATFORM} TRUE)" in source
source = source.replace(needle,
    "  if(ZC_PS5_SDL_TARGET)\n    set(SDL_CMAKE_PLATFORM PS5)\n"
    "  elseif(WIN32)\n    set(SDL_CMAKE_PLATFORM WINDOWS)", 1)
platform.write_text(source)

# PS5 libc supplies mathematical functions in libSceLibcInternal, not
# libm. SDL2's generic UNIX probe finds math.h and appends a nonexistent
# '-lm' linker dependency. Retain real math calls, replace only the
# platform library list entry when compiling for the actual PS5 backend.
cmake=args.source/"CMakeLists.txt"
text=cmake.read_text()
old="list(APPEND EXTRA_LIBS m)"
assert text.count(old)==1, "Pinned SDL2 libm logic changed"
text=text.replace(old, """if(PS5)
          list(APPEND EXTRA_LIBS SceLibcInternal)
        else()
          list(APPEND EXTRA_LIBS m)
        endif()""",1)
cmake.write_text(text)


# The PS5 SDK provides C math/thread APIs through system imports; it does
# not package separate Linux/BSD libm or libpthread linker archives.
# Strip only those *link options*, not source code or unresolved symbols.
cmake = args.source / "CMakeLists.txt"
text = cmake.read_text()
target_link = "  target_link_libraries(SDL2-static PRIVATE ${EXTRA_LIBS} ${EXTRA_LDFLAGS} ${CMAKE_DEPENDS})"
assert text.count(target_link)==1, "Pinned SDL2-static link line changed"
ps5_link = """  if(ZC_PS5_SDL_TARGET)
    foreach(_ps5_systemlib IN ITEMS m pthread -lm -lpthread -pthread SceSystemService ScePad SceUserService SceAudioOut SceVideoOut SceKeyboard SceImeDialog samplerate)
      list(REMOVE_ITEM EXTRA_LIBS "${_ps5_systemlib}")
      list(REMOVE_ITEM EXTRA_LDFLAGS "${_ps5_systemlib}")
      list(REMOVE_ITEM CMAKE_DEPENDS "${_ps5_systemlib}")
    endforeach()
  endif()
"""
text=text.replace(target_link, ps5_link+target_link,1)
cmake.write_text(text)


cmake = args.source / "CMakeLists.txt"
text = cmake.read_text()
assert "elseif(PS5)" in text
assert "SDL_JOYSTICK_PS5" in text
assert "SDL_AUDIO_DRIVER_PS5" in text
assert "SDL_VIDEO_DRIVER_PS5" in text
assert "SDL_FILESYSTEM_PS5" in text
print("PS5 SDL2 CMake platform selection enabled: real audio/video/pad backend")


# PS5's libc headers advertise several BSD wchar APIs which are not exported
# by the actual firmware import stubs. SDL already has portable implementations
# for these four functions. Select SDL's real fallback implementation rather
# than fabricating missing libc symbols.
strings=args.source/"src/stdlib/SDL_string.c"
content=strings.read_text()
for macro in ("HAVE_WCSLCPY","HAVE_WCSLCAT","HAVE_WCSCASECMP","HAVE_WCSNCASECMP"):
    original=f"#if defined({macro})"
    assert content.count(original)==1, f"Pinned SDL wchar guard changed: {macro}"
    content=content.replace(original, f"#if defined({macro}) && !defined(ZC_PS5_WCHAR_FALLBACK)",1)
strings.write_text(content)
print("PS5 SDL wchar functions use genuine SDL fallback implementations")
