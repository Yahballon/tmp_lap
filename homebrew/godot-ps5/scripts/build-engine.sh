#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
: "${GODOT_SOURCE:?Point GODOT_SOURCE to an upstream godot checkout (4.7.2-stable)}"
: "${PS5_PAYLOAD_SDK:?Point PS5_PAYLOAD_SDK to DevBridge's payload SDK}"
test -f "$GODOT_SOURCE/SConstruct"
test -d "$PS5_PAYLOAD_SDK/target/include/c++/v1"
python3 "$root/scripts/check_overlay.py"
cp -a "$root/engine/platform/ps5" "$GODOT_SOURCE/platform/"
cd "$GODOT_SOURCE"
scons platform=ps5 target=template_debug library_type=static_library tools=no arch=x86_64 lto=none vulkan=no opengl3=no -j"${BUILD_JOBS:-2}"
echo 'EXPERIMENTAL static engine compilation completed, NOT a playable PS5 homebrew package.'
