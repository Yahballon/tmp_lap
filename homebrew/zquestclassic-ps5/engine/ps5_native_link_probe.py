#!/usr/bin/env python3
"""Make the Ninja *diagnostic* zplayer link use real PS5 SDK .so imports.

This script never creates stub symbols or ignores undefined references. It
replaces desktop -pthread/-lm with real PS5 native libc/kernel imports, and
maps SDL2's -lSce* names to actual SDK-provided shared import libraries.
The normal PS5-DevBridge build links target/lib/*.so directly; CMake/SDL2
cross-link does not do this automatically. Production final packaging still
needs the PS5-DevBridge CRT/linker/FSELF pipeline and console validation.
"""
import argparse
from pathlib import Path
import shlex

argp = argparse.ArgumentParser()
argp.add_argument("ninja", type=Path)
argp.add_argument("sdk_lib", type=Path)
args = argp.parse_args()
assert args.ninja.is_file()
assert args.sdk_lib.is_dir()
real_imports = sorted(args.sdk_lib.glob("*.so"))
assert len(real_imports) >= 10, "Missing real PS5 SDK system imports"
for name in ("libkernel_web.so", "libSceLibcInternal.so", "libScePad.so",
             "libSceUserService.so", "libSceSystemService.so"):
    assert (args.sdk_lib/name).is_file(), f"Missing genuine SDK import: {name}"

lines = args.ninja.read_text().splitlines(keepends=True)
builds = [i for i,s in enumerate(lines) if s.startswith("build zplayer: ")]
assert len(builds) == 1, "Expected exactly one upstream zplayer target"
start = builds[0] + 1
end = next((i for i in range(start,len(lines))
            if lines[i].startswith("build ") or not lines[i].strip()),len(lines))
props = [i for i in range(start,end)
         if lines[i].startswith("  LINK_LIBRARIES = ")]
assert len(props) == 1, "Expected one generated zplayer link-library field"
i = props[0]
pre = "  LINK_LIBRARIES = "
original = lines[i][len(pre):].rstrip("\n")
tokens = shlex.split(original)
converted = []
discarded = []
mapped = []
for token in tokens:
    if token in ("-lm", "-lpthread", "-pthread"):
        # These Linux/BSD library filenames do not exist on native PS5.
        # Their ACTUAL required functions must be satisfied by the imported
        # PS5 libc/kernel below; undefined references still fail the link.
        discarded.append(token)
    elif token.startswith("-lSce"):
        target = args.sdk_lib/("lib"+token[2:]+".so")
        assert target.is_file(), f"No genuine PS5 import for {token}"
        converted.append(str(target))
        mapped.append(token)
    else:
        converted.append(token)
assert mapped, "Expected actual upstream SDL2 Sony import dependencies"
# Match the working homebrew linking strategy: genuine SDK .so imports.
# No undef-suppression, fake symbols, empty archives or substitute drivers.
seen = set(converted)
for path in real_imports:
    if str(path) not in seen:
        converted.append(str(path))
lines[i] = pre + " ".join(converted) + "\n"
args.ninja.write_text("".join(lines))
print("Real PS5 SDK imports:", len(real_imports))
print("Converted SDL2 PS5 imports:", ", ".join(mapped))
print("Removed non-existent desktop library names:", ", ".join(discarded))
print("Undefined symbols remain fatal. This is only a link probe.")
