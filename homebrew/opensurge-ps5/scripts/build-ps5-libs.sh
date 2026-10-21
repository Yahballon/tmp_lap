#!/usr/bin/env bash
# Cross-compile the upstream full SurgeScript runtime and PhysFS as PS5
# static libraries, without faking a playable game or package.
set -euo pipefail
project=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
: "${PS5_DEVBRIDGE_TEMPLATE_ROOT:?Set PS5_DEVBRIDGE_TEMPLATE_ROOT to simple-calc SDK template}"
export PS5_PAYLOAD_SDK="${PS5_PAYLOAD_SDK:-$PS5_DEVBRIDGE_TEMPLATE_ROOT/.deps/native/ps5-payload-sdk}"
export PS5_CC="${PS5_CC:-$PS5_DEVBRIDGE_TEMPLATE_ROOT/tooling/prospero-clang18}"
export PS5_CLANG="${PS5_CLANG:-clang-18}"
export USE_CCACHE=0

for command in cmake git llvm-ar-18 llvm-ranlib-18 "$PS5_CLANG"; do
  command -v "$command" >/dev/null || { echo "Missing: $command" >&2; exit 2; }
done
test -s "$PS5_PAYLOAD_SDK/target/lib/libc.a" || { echo "Missing PS5 target SDK" >&2; exit 2; }
test -f "$PS5_CC" || { echo "Missing PS5 Clang wrapper" >&2; exit 2; }
# The upstream template invokes this wrapper with sh; CMake invokes its compiler directly.
# Mark only our isolated temporary copy executable, never change the SDK repository.
chmod u+x "$PS5_CC"
test -x "$PS5_CC" || { echo "PS5 compiler wrapper is not executable" >&2; exit 2; }

work=${1:-"$project/.work"}
bash "$project/scripts/fetch-upstream.sh" "$work"
python3 "$project/scripts/patch-surgescript-ps5.py" "$work/surgescript"
prefix="$work/ps5-port-prefix"
toolchain="$project/ps5/ps5-static-toolchain.cmake"
mkdir -p "$prefix"

cmake -S "$work/surgescript" -B "$work/build-surgescript-ps5" \
  -DCMAKE_TOOLCHAIN_FILE="$toolchain" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_INSTALL_PREFIX="$prefix" \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-DOPENSURGE_PS5" -DWANT_SHARED=OFF -DWANT_STATIC=ON -DWANT_EXECUTABLE=OFF
cmake --build "$work/build-surgescript-ps5" --parallel "${BUILD_JOBS:-4}" --target surgescript-static
cmake --install "$work/build-surgescript-ps5"

cmake -S "$work/physfs" -B "$work/build-physfs-ps5" \
  -DCMAKE_TOOLCHAIN_FILE="$toolchain" -DCMAKE_INSTALL_PREFIX="$prefix" \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-DPHYSFS_NO_CDROM_SUPPORT=1" \
  -DPHYSFS_BUILD_SHARED=OFF -DPHYSFS_BUILD_STATIC=ON \
  -DPHYSFS_BUILD_TEST=OFF -DPHYSFS_BUILD_DOCS=OFF -DPHYSFS_INSTALL=ON
cmake --build "$work/build-physfs-ps5" --parallel "${BUILD_JOBS:-4}" --target physfs-static
cmake --install "$work/build-physfs-ps5"

test -s "$prefix/lib/libsurgescript-static.a"
test -s "$prefix/lib/libphysfs.a"
echo "PASS: cross-compiled PS5-target C libraries, no playable native game claimed."
