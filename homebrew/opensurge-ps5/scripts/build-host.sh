#!/usr/bin/env bash
# Build the REAL upstream SurgeScript runtime and complete Open Surge engine
# on Linux as a source integration/compatibility check. NOT a PS5 binary.
set -euo pipefail
project=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
work=${1:-"$project/.work"}
bash "$project/scripts/fetch-upstream.sh" "$work"
prefix="$work/host-prefix"
cmake -S "$work/surgescript" -B "$work/build-surgescript-host" \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix" \
  -DWANT_EXECUTABLE=OFF -DWANT_SHARED=ON -DWANT_STATIC=OFF
cmake --build "$work/build-surgescript-host" --parallel "${BUILD_JOBS:-4}"
cmake --install "$work/build-surgescript-host"
cmake -S "$work/opensurge" -B "$work/build-opensurge-host" \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$prefix" \
  -DCMAKE_PREFIX_PATH="$prefix" \
  -DCMAKE_INCLUDE_PATH="$prefix/include" -DCMAKE_LIBRARY_PATH="$prefix/lib" \
  -DWANT_BETTER_GAMEPAD=OFF -DGAME_RUNINPLACE=OFF
cmake --build "$work/build-opensurge-host" --parallel "${BUILD_JOBS:-4}"
test -s "$work/build-opensurge-host/opensurge"
LD_LIBRARY_PATH="$prefix/lib:${LD_LIBRARY_PATH:-}" "$work/build-opensurge-host/opensurge" --help >/dev/null
echo 'PASS: original Open Surge engine + SurgeScript built on Linux (NOT PS5)'
