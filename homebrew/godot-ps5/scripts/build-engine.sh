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
python3 "$root/scripts/patch_ps5_kvm.py" "$GODOT_SOURCE"
python3 "$root/scripts/patch_ps5_setup_trace.py" "$GODOT_SOURCE"
python3 "$root/scripts/patch_ps5_post_settings_trace.py" "$GODOT_SOURCE"
python3 "$root/scripts/patch_ps5_userdata.py" "$GODOT_SOURCE"
python3 "$root/scripts/patch_ps5_dir_access.py" "$GODOT_SOURCE"
cd "$GODOT_SOURCE"
jobs=4
if test -n "$(printenv BUILD_JOBS || :)"; then jobs="$(printenv BUILD_JOBS)"; fi
# A first HEADLESS GDScript smoke test should not link Embree / Jolt /
# 3D render importers. Building all modules worked, but the final PS5 link
# failed on optional Embree symbols. The full engine can be re-enabled with
# GODOT_MODULE_SET=full after core runtime execution is proven.
module_flags=()
case "${GODOT_MODULE_SET:-smoke}" in
  smoke)
    module_flags=(modules_enabled_by_default=no
                  module_gdscript_enabled=yes
                  module_text_server_fb_enabled=yes
                  disable_3d=yes
                  disable_xr=yes
                  disable_advanced_gui=yes)
    ;;
  full)
    ;;
  *)
    echo 'GODOT_MODULE_SET must be smoke or full' >&2
    exit 2
    ;;
esac
# Only CI's opt-in GLES3 archive gate can change this default.
gl_backend=no
if [[ "${GODOT_PS5_GLES3_ARCHIVES:-0}" == "1" ]]; then gl_backend=yes; fi
scons platform=ps5 target=template_debug library_type=static_library \
    arch=x86_64 lto=none vulkan=no opengl3="$gl_backend" d3d12=no metal=no \
    module_mono_enabled=no use_llvm=yes disable_path_overrides=no "${module_flags[@]}" -j"$jobs"
echo 'Engine archive build completed: NOT an installable PS5 title.'
