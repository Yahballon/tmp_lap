#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Probe real upstream zplayer target with actual PS5 toolchain.
# A failing job is informative. Never publish a zip from this workflow.
set -euo pipefail
root="$(cd -- "$(dirname "$0")/../../.." && pwd)"
work="${RUNNER_TEMP:-/tmp}/zcps5-full-zplayer"
mkdir -p "$work"
upstream="$work/upstream"
sdl="$work/sdl2-ps5"
allegro="$work/allegro5-ps5-probe"
build="$work/build"
report="$work/results.txt"
log="$work/configure.log"
echo "Original upstream zplayer PS5 cross-compilation, not playable" > "$report"
test -d "$upstream/.git"
test "$(git -C "$upstream" rev-parse HEAD)" = "c44dde7d0146da78bf8b541af457d5ff4e67a830"
test "$(git -C "$sdl" rev-parse HEAD)" = "ee4c47dc0d617b3bc8f35108f9956baf228a1322"
test "$(git -C "$allegro" rev-parse HEAD)" = "aae80ca3573e19702cfc2338591a575dc75eb647"
test -f "$upstream/src/zc/zelda.cpp"
test -f "$upstream/src/core/qst.cpp"
test -f "$sdl/src/video/ps5/SDL_ps5video.c"
test -f "$sdl/src/audio/ps5/SDL_ps5audio.c"
test -f "$root/homebrew/zquestclassic-ps5/engine/ps5_zplayer_toolchain.cmake"
test -f "${PS5_PAYLOAD_SDK:?}/target/include/c++/v1/vector"
test -f "${PS5_DEVBRIDGE_TOOL:?}"
test -x "$PS5_PAYLOAD_SDK/bin/prospero-lld"
test -d "$PS5_PAYLOAD_SDK/target/lib"
find "$PS5_PAYLOAD_SDK/target/lib" -maxdepth 1 -type f \( -name "*.so" -o -name "*.a" \) | head -n 20
export PATH="$PS5_PAYLOAD_SDK/bin:$PATH"
test -f "${PS5_OPENGL_PREFIX:?}/lib/libPS5OpenGL.a"
# A real PS5 linker must see the actual SDK's Sony import stubs.
test -f "$PS5_PAYLOAD_SDK/target/lib/libSceLibcInternal.so"
test -f "$PS5_PAYLOAD_SDK/target/lib/libScePad.so"
test -f "$PS5_PAYLOAD_SDK/target/lib/libSceSystemService.so"

test -f "$PS5_OPENGL_PREFIX/include/GL/glcorearb.h"
# Native PS5 SDK provides linkable system imports as .so files. The generic
# CMake clang driver does not add this directory automatically. Never
# substitute dummy ScePad/UserService/SystemService libraries.
for lib in libScePad.so libSceUserService.so libSceSystemService.so; do
  test -f "$PS5_PAYLOAD_SDK/target/lib/$lib" || {
    echo "Required native PS5 SDK import missing: $lib" >&2
    exit 4
  }
done
test -d "$PS5_PAYLOAD_SDK/target/lib"
echo "PS5 import libraries present:" >> "$report"
find "$PS5_PAYLOAD_SDK/target/lib" -maxdepth 1 -type f \
  \( -name "libSce*.so" -o -name "libm*" -o -name "libpthread*" \) \
  -printf "%f\n" | sort | sed -n "1,40p" >> "$report"
platform="$work/ps5-platform"
test "$(git -C "$platform" rev-parse HEAD)" = "b5efad528e0ac1b8289f39d72a0e890ff68ff0a6"
export ZC_PS5_PLATFORM_ARCHIVE="$platform/platform/build/ps5/libzcps5locale.a"
export ZC_PS5_ASSERT_SOURCE="$root/homebrew/zquestclassic-ps5/engine/ps5_assert_report.c"
test -f "$ZC_PS5_ASSERT_SOURCE"
test -s "$ZC_PS5_PLATFORM_ARCHIVE"
# The payload SDK imports system libraries as .so stubs, unlike Linux
# libm/libpthread. Verify real stubs before running the link.
for module in libSceSystemService.so libScePad.so libSceUserService.so; do
  test -f "$PS5_PAYLOAD_SDK/target/lib/$module" || {
    echo "MISSING_REAL_PS5_IMPORT $module" >&2
    exit 33
  }
done
test -d "$PS5_PAYLOAD_SDK/target/lib"
find "$PS5_PAYLOAD_SDK/target/lib" -maxdepth 1 -name 'libSce*.so' -printf '[zcps5] SDK module: %f\n' | head -n 12 || true
echo "Original zplayer: c44dde7d0146da78bf8b541af457d5ff4e67a830" >> "$report"
echo "SDL2 PS5 fork: ee4c47dc0d617b3bc8f35108f9956baf228a1322" >> "$report"
echo "Attempt original zplayer with ALLEGRO_SDL=ON and JIT_BACKEND=none" >> "$report"
echo "Locale: genuine Mihawk PS5 libc compat b5efad528e0ac1b8289f39d72a0e890ff68ff0a6" >> "$report"
python3 "$root/homebrew/zquestclassic-ps5/engine/prepare_upstream_cmake_for_ps5.py" "$upstream"
python3 "$root/homebrew/zquestclassic-ps5/engine/prepare_zconsole_for_ps5.py" "$upstream"
python3 "$root/homebrew/zquestclassic-ps5/engine/prepare_allegro_for_ps5_probe.py" "$allegro"
python3 "$root/homebrew/zquestclassic-ps5/engine/prepare_sdl2_for_ps5_probe.py" "$sdl"
echo "PS5 adaptations: portable stduuid; WANT_NFD=OFF" >> "$report"
# The generator must receive every -D flag as a single shell command.
# Avoid accidental blank lines after a continuation backslash.
bash -n "$root/homebrew/zquestclassic-ps5/engine/ps5_zplayer_full_probe.sh"
echo "[zcps5] Actual upstream CMake configure, not a dummy target"
set +e
timeout 600 cmake -S "$upstream" -B "$build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  "-DCMAKE_EXE_LINKER_FLAGS=-L$PS5_PAYLOAD_SDK/target/lib -Wl,--error-limit=100" \
  -DCMAKE_HAVE_LIBC_PTHREAD=TRUE \
  -DTHREADS_PREFER_PTHREAD_FLAG=OFF \
  "-DCMAKE_C_FLAGS_RELEASE=-O1 -DNDEBUG" \
  "-DCMAKE_CXX_FLAGS_RELEASE=-O1 -DNDEBUG" \
  "-DCMAKE_C_FLAGS=-DZC_PS5_LIBC_NO_STRERROR_S=1 -DZC_PS5_NO_GLU_HEADER=1 -DZC_PS5_WCHAR_FALLBACK=1" \
  "-DCMAKE_CXX_FLAGS=-fexceptions -frtti" \
  "-DZC_PS5_LIBC_LINK_PROBE=ON" \
  -DCMAKE_TOOLCHAIN_FILE="$root/homebrew/zquestclassic-ps5/engine/ps5_zplayer_toolchain.cmake" \
  -DFETCHCONTENT_SOURCE_DIR_SDL2="$sdl" \
  -DSDL2_INCLUDE_DIR="$sdl/include" \
  -DSDL2_LIBRARY=SDL2-static \
  -DFETCHCONTENT_SOURCE_DIR_ALLEGRO5="$allegro" \
  -DZC_PS5_NO_GLU_PROBE=ON \
  -DZC_PS5_AUDIO_DRIVER_PROBE=ON \
  -DZC_PS5_TARGET_ONLY_PROBE=ON \
  -DZC_PS5_SDL_TARGET=ON \
  -DZC_PS5_NO_DESKTOP_UNIX_LIBS=ON \
  -DALLEGRO_LEGACY_USE_CONSTRUCTOR=0 \
  -DOpenGL_GL_PREFERENCE=LEGACY \
  -DOPENGL_INCLUDE_DIR="$PS5_OPENGL_PREFIX/include" \
  -DOPENGL_gl_LIBRARY="$PS5_OPENGL_PREFIX/lib/libPS5OpenGL.a" \
  -DALLEGRO_SDL=ON \
  -DWANT_NFD=OFF \
  -DWANT_WEBSOCKETS=OFF \
  -DCOPY_RESOURCES=OFF \
  -DJIT_BACKEND=none >"$log" 2>&1
status=$?
set -e
echo "CMake configure code: $status" >> "$report"
tail -n 65 "$log" || true
if ((status != 0)); then
  echo "BLOCKED: real zplayer CMake configuration (see configure.log)" >> "$report"
  exit 31
fi
echo "[zcps5] Attempting original zplayer CMake target"
set +e
timeout 1050 cmake --build "$build" --target zplayer --parallel 3 -- -k 8 >"$work/compile.log" 2>&1
status=$?
set -e
echo "zplayer build code: $status" >> "$report"
grep -E '^FAILED:|(^|[[:space:]])(fatal )?error:|undefined reference|ninja: build stopped' "$work/compile.log" | head -n 80 >>"$report" || true
tail -n 65 "$work/compile.log" || true
if ((status != 0)); then
  echo "BLOCKED: original zplayer cannot yet compile/link (see compile.log)" >> "$report"
  exit 32
fi
echo "Real upstream zplayer compiled; PS5 ELF packaging and hardware verification pending." >> "$report"
