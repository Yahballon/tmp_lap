#!/usr/bin/env bash
# Native cross-compile entrypoint. Fails CLOSED until real target libraries
# (especially Allegro 5 using a working PS5 SDL2/OpenGL backend) exist.
set -euo pipefail
project=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
: "${PS5_TOOLCHAIN_FILE:?Set PS5_TOOLCHAIN_FILE to the verified PS5 CMake toolchain}"
: "${PS5_PORT_PREFIX:?Set PS5_PORT_PREFIX to a PS5-targeted dependencies sysroot}"
for filename in include/allegro5/allegro.h include/surgescript.h include/physfs.h \
  lib/liballegro.a lib/liballegro_image.a lib/liballegro_audio.a \
  lib/liballegro_acodec.a lib/liballegro_font.a lib/liballegro_ttf.a \
  lib/liballegro_primitives.a lib/liballegro_memfile.a \
  lib/liballegro_physfs.a lib/liballegro_color.a \
  lib/liballegro_dialog.a lib/liballegro_main.a \
  lib/libsurgescript-static.a lib/libphysfs.a; do
  test -s "$PS5_PORT_PREFIX/$filename" || {
    echo "PS5 dependency missing: $PS5_PORT_PREFIX/$filename" >&2
    echo "No installable PS5 game will be generated." >&2
    exit 3
  }
done

work=${1:-"$project/.work"}
bash "$project/scripts/fetch-upstream.sh" "$work"
python3 "$project/scripts/patch-ps5.py" "$work/opensurge"
cmake -S "$work/opensurge" -B "$work/build-opensurge-ps5" \
  -DCMAKE_TOOLCHAIN_FILE="$PS5_TOOLCHAIN_FILE" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$PS5_PORT_PREFIX" \
  -DCMAKE_INCLUDE_PATH="$PS5_PORT_PREFIX/include" \
  -DCMAKE_LIBRARY_PATH="$PS5_PORT_PREFIX/lib" \
  -DCMAKE_C_FLAGS="-DOPENSURGE_PS5" \
  -DGAME_RUNINPLACE=ON -DSURGESCRIPT_STATIC=ON \
  -DWANT_BETTER_GAMEPAD=OFF
cmake --build "$work/build-opensurge-ps5" --parallel "${BUILD_JOBS:-4}"
echo "Cross-linked engine only. PS5 native ELF signing, module bundling,"
echo "icon/asset install layout and actual hardware test still required."
