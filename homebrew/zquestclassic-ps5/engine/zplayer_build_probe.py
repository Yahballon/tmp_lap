#!/usr/bin/env python3
"""Inspect genuine upstream CMake zplayer target, not a fake playable build."""
import argparse
import pathlib
import re

parser = argparse.ArgumentParser()
parser.add_argument("upstream_root", type=pathlib.Path)
args = parser.parse_args()
root = args.upstream_root
cmake = (root / "CMakeLists.txt").read_text()
main = (root / "src/zc/zelda.cpp").read_text()
assert "add_executable(zplayer " in cmake
assert re.search(r"target_link_libraries\(zplayer PRIVATE core zcsound", cmake)
assert re.search(r"int main\(int argc, char \*\*argv\)", main)
assert 'zapp_check_switch("-standalone", {"qst"})' in main
assert "target_sources(zplayer PUBLIC src/zc/scripting/jit/jit_none.cpp)" in cmake
required = ("src/core/qst.cpp", "src/core/qst_header.cpp",
            "src/zc/zelda.cpp", "src/components/zasm/table.cpp",
            "src/zc/scripting/jit/jit_none.cpp")
for path in required:
    assert (root / path).is_file(), path
print("Verified real zplayer and its interpreter-only backend; not linked on PS5")
