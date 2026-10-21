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
build="$work/build"
report="$work/results.txt"
log="$work/configure.log"
echo "Original upstream zplayer PS5 cross-compilation, not playable" > "$report"
test -d "$upstream/.git"
test "$(git -C "$upstream" rev-parse HEAD)" = "c44dde7d0146da78bf8b541af457d5ff4e67a830"
test "$(git -C "$sdl" rev-parse HEAD)" = "ee4c47dc0d617b3bc8f35108f9956baf228a1322"
test -f "$upstream/src/zc/zelda.cpp"
test -f "$upstream/src/core/qst.cpp"
test -f "$sdl/src/video/ps5/SDL_ps5video.c"
test -f "$sdl/src/audio/ps5/SDL_ps5audio.c"
test -f "$root/homebrew/zquestclassic-ps5/engine/ps5_zplayer_toolchain.cmake"
test -f "${PS5_PAYLOAD_SDK:?}/target/include/c++/v1/vector"
test -f "${PS5_DEVBRIDGE_TOOL:?}"
test -f "${PS5_OPENGL_PREFIX:?}/lib/libPS5OpenGL.a"
test -f "$PS5_OPENGL_PREFIX/include/GL/glcorearb.h"
echo "Original zplayer: c44dde7d0146da78bf8b541af457d5ff4e67a830" >> "$report"
echo "SDL2 PS5 fork: ee4c47dc0d617b3bc8f35108f9956baf228a1322" >> "$report"
echo "Attempt original zplayer with ALLEGRO_SDL=ON and JIT_BACKEND=none" >> "$report"
python3 "$root/homebrew/zquestclassic-ps5/engine/prepare_upstream_cmake_for_ps5.py" "$upstream"
echo "PS5 adaptations: portable stduuid; WANT_NFD=OFF" >> "$report"
echo "[zcps5] Actual upstream CMake configure, not a dummy target"
set +e
timeout 600 cmake -S "$upstream" -B "$build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$root/homebrew/zquestclassic-ps5/engine/ps5_zplayer_toolchain.cmake" \
  -DFETCHCONTENT_SOURCE_DIR_SDL2="$sdl" \
  -DOpenGL_GL_PREFERENCE=LEGACY \
  -DOPENGL_INCLUDE_DIR="$PS5_OPENGL_PREFIX/include" \
  -DOPENGL_gl_LIBRARY="$PS5_OPENGL_PREFIX/lib/libPS5OpenGL.a" \
  -DALLEGRO_SDL=ON \
  -DWANT_NFD=OFF \
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
timeout 480 cmake --build "$build" --target zplayer --parallel 3 >"$work/compile.log" 2>&1
status=$?
set -e
echo "zplayer build code: $status" >> "$report"
tail -n 65 "$work/compile.log" || true
if ((status != 0)); then
  echo "BLOCKED: original zplayer cannot yet compile/link (see compile.log)" >> "$report"
  exit 32
fi
echo "Real upstream zplayer compiled; PS5 ELF packaging and hardware verification pending." >> "$report"
