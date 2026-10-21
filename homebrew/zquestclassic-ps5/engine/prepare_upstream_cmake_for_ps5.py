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
cmake.write_text(text.replace(old,new,1))
print("PS5 staging: stduuid portable generator, no libuuid dependency")
