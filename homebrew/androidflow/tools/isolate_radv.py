#!/usr/bin/env python3
"""Privatize Mesa RADV implementation symbols to coexist with ps5-opengl Mesa.

Adapted in approach from Swordpdf/ProsperoEden/tools/isolate-radv.py
(GPL-3.0-or-later). Never mutates the source archive.
"""
from pathlib import Path
import hashlib
import json
import os
import re
import subprocess

vk = Path(os.environ.get("PS5_VULKAN", str(Path.home() / "ps5/PS5_Vulkan")))
source = vk / ".deps/native/radv-release/lib/libvulkan_radeon.ps5.a"
out = Path(os.environ.get("RADV_ISOLATED", str(Path.home() / "ps5/ps5droid/build-radv-isolated")))
out.mkdir(parents=True, exist_ok=True)
if not source.is_file():
    raise SystemExit(f"Missing RADV release archive: {source}")

def symbols(path, defined=False):
    cmd = ["nm", "-g", "--format=posix"]
    if defined:
        cmd.append("--defined-only")
    content = subprocess.check_output(cmd + [str(path)], text=True)
    return {parts[0] for line in content.splitlines()
            if len(parts := line.split()) >= 2 and len(parts[1]) == 1}

def sha256(path):
    with open(path, "rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()

names = sorted(symbols(source, True))
demangled = subprocess.check_output(
    ["c++filt"], input="\n".join(names)+"\n", text=True).splitlines()
assert len(names) == len(demangled)
public = {s for s in names if re.match(r"vk[A-Z]|vk_icd", s)}
public.add("radv_GetInstanceProcAddr")
private = []
for name, readable in zip(names, demangled):
    shared = ((re.match(r"^(typeinfo (for|name for) std::|vtable for std::)", readable)
               and "<" not in readable)
              or readable.startswith(("operator new", "operator delete"))
              or name.startswith(("__cxa_", "__gxx_", "_Unwind_")))
    if name not in public and not shared:
        private.append(name)
mapping = out / "symbols.map"
mapping.write_text("".join(f"{name} androidflow_radv_private_{name}\n" for name in private))
before = sha256(source)
dest = out / source.name
subprocess.run(["objcopy", "--redefine-syms="+str(mapping),
                str(source), str(dest)], check=True)
subprocess.run(["ranlib", str(dest)], check=True)
assert sha256(source) == before, "RADV source archive changed unexpectedly"
assert not set(private).intersection(symbols(dest)), "RADV namespace collision still exists"
assert public.intersection(names) <= symbols(dest, True), "Public Vulkan entrypoint lost"
(out / "manifest.json").write_text(json.dumps({
    "source": str(source),
    "source_sha256": before,
    "isolated_sha256": sha256(dest),
    "private_symbols": len(private),
    "preserved_public": sorted(public.intersection(names)),
}, indent=2)+"\n")
print(f"AndroidFlow: isolated {len(private)} RADV implementation symbols in {dest}")
