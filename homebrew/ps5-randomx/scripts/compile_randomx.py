#!/usr/bin/env python3
"""Cross-compile pinned upstream RandomX v1.2.1 to a PS5 static archive.
No stubs or simulated hashes: src/randomx.cpp and interpreter actually run.
"""
import concurrent.futures
import os
from pathlib import Path
import subprocess
import sys

app = Path(sys.argv[1]).resolve()
upstream = Path(sys.argv[2]).resolve()
root = app / "build" / "randomx"
root.mkdir(parents=True, exist_ok=True)
sdk = app / ".deps" / "native" / "ps5-payload-sdk"
wrapper = app / "tooling" / "prospero-clang18"
assert (sdk / "target" / "include").exists(), "PS5 SDK was not initialized"
assert (upstream / "src" / "randomx.h").exists(), "RandomX checkout is missing"
sources = """
aes_hash.cpp argon2_ref.c argon2_ssse3.c argon2_avx2.c
bytecode_machine.cpp cpu.cpp dataset.cpp soft_aes.cpp virtual_memory.c
vm_interpreted.cpp allocator.cpp assembly_generator_x86.cpp
instruction.cpp randomx.cpp superscalar.cpp vm_compiled.cpp
vm_interpreted_light.cpp argon2_core.c blake2_generator.cpp
instructions_portable.cpp reciprocal.c virtual_machine.cpp
vm_compiled_light.cpp blake2/blake2b.c jit_compiler_x86.cpp
jit_compiler_x86_static.S
""".split()
env = os.environ.copy()
env["PS5_PAYLOAD_SDK"] = str(sdk)
env["PS5_CLANG"] = "clang-18"
env["USE_CCACHE"] = "0"

def build(item):
    source = upstream / "src" / item
    obj = root / (item.replace("/", "_") + ".o")
    is_cpp = item.endswith(".cpp")
    is_asm = item.endswith(".S")
    flags = ["-O2", "-fPIC", "-fno-stack-protector", "-I" + str(upstream / "src"),
             "-DHAVE_POSIX_MEMALIGN=1", "-c", str(source), "-o", str(obj)]
    if is_cpp:
        flags = ["-std=c++11", "-fexceptions", "-frtti", "-maes"] + flags
    elif not is_asm:
        flags = ["-std=c11"] + flags
    if "argon2_ssse3" in item: flags.insert(0, "-mssse3")
    if "argon2_avx2" in item: flags.insert(0, "-mavx2")
    if is_asm: flags.insert(0, "-xassembler-with-cpp")
    subprocess.run(["sh", str(wrapper)] + flags, cwd=app, env=env, check=True)
    return str(obj)

with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
    objects = list(pool.map(build, sources))
archive = root / "librandomx.a"
subprocess.run(["llvm-ar-18", "rcs", str(archive)] + objects, check=True)
print("Cross compiled real RandomX archive:", archive, archive.stat().st_size)
