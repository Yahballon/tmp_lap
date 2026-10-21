#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
: "$GODOT_SOURCE"
: "$PS5_PAYLOAD_SDK"
test -f "$GODOT_SOURCE/SConstruct"
test -d "$PS5_PAYLOAD_SDK/target/include/c++/v1"
python3 "$root/scripts/check_overlay.py"
mkdir -p "$GODOT_SOURCE/platform/ps5"
cp -a "$root/engine/platform/ps5/." "$GODOT_SOURCE/platform/ps5/"
cd "$GODOT_SOURCE"
jobs=4
if test -n "$(printenv BUILD_JOBS || :)"; then jobs="$(printenv BUILD_JOBS)"; fi
scons platform=ps5 target=template_debug library_type=static_library \
    arch=x86_64 lto=none vulkan=no opengl3=no d3d12=no metal=no \
    module_mono_enabled=no use_llvm=yes -j"$jobs"
echo 'Engine archive build completed: NOT an installable PS5 title.'
