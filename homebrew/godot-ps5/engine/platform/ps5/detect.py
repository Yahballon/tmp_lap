# SPDX-License-Identifier: MIT
"""Experimental Godot 4.7.2 static engine target for PS5 homebrew."""
import os
from pathlib import Path
from methods import print_error
from platform_methods import validate_arch

def get_name():
    return "PS5 Homebrew (experimental)"

def can_build():
    sdk = os.environ.get("PS5_PAYLOAD_SDK")
    return bool(sdk and Path(sdk, "target/include").is_dir())

def get_opts():
    return []

def get_flags():
    return {
        "arch": "x86_64", "target": "template_debug",
        "vulkan": False, "opengl3": False, "d3d12": False,
        "metal": False, "accesskit": False, "sdl": False,
        "builtin_pcre2_with_jit": False,
        "supported": ["library"],
    }

def configure(env):
    validate_arch(env["arch"], get_name(), ["x86_64"])
    if env["library_type"] != "static_library":
        print_error("PS5: static library required in the first milestone")
        raise SystemExit(2)
    sdk = Path(os.environ["PS5_PAYLOAD_SDK"]).resolve()
    if not (sdk / "target/include/c++/v1").is_dir():
        print_error("PS5: missing target libc++ headers")
        raise SystemExit(2)
    env["CC"] = os.environ.get("PS5_CLANG", "clang-18")
    env["CXX"] = env["CC"]
    env["AR"] = os.environ.get("PS5_AR", "llvm-ar-18")
    env["RANLIB"] = os.environ.get("PS5_RANLIB", "llvm-ranlib-18")
    env.Append(CCFLAGS=[
        "-target", "x86_64-sie-ps5",
        "-fvisibility-nodllstorageclass=default",
        "-isysroot", str(sdk),
        "-isystem", str(sdk / "target/include/c++/v1"),
        "-isystem", str(sdk / "target/include"),
        "-fno-stack-protector", "-fno-plt", "-femulated-tls",
        "-fno-omit-frame-pointer", "-fno-exceptions",
    ])
    # PS5 toolchain defaults to RTTI-off; Godot 4 uses typeid in its core.
    env.Append(CXXFLAGS=["-frtti"])
    env.Append(CPPDEFINES=["PS5_ENABLED", "UNIX_ENABLED",
                           "GODOT_PS5_NATIVE_IO", ("_FILE_OFFSET_BITS", 64)])
    env.Append(CPPPATH=["#platform/ps5"])
