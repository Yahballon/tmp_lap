#!/usr/bin/env bash
# Cross-compile experiment only: do NOT put output into public PS5 release.
set -Eeuo pipefail
source_root="${SOURCE_ROOT:-$PWD}"
root="${WORK_ROOT:-/tmp/piper-onnx-ps5}"
mkdir -p "$root"
exec > >(tee "$root/probe.log") 2>&1
trap 'echo "Native ONNX probe failed at line $LINENO (exit $?)"' ERR
export DEBIAN_FRONTEND=noninteractive

echo "[PROBE] Preparing independent PS5 SDK"
git clone --depth=1 https://github.com/Yahballon/PS5-DevBridge.git "$root/devbridge"
cd "$root/devbridge/templates/simple-calc"
bash tools/setup-native-dependencies.sh
sdk="$PWD/.deps/native/ps5-payload-sdk"
test -s "$sdk/target/lib/libc.a"
test -d "$sdk/target/include"
export PS5_HOME="$sdk"

echo "[PROBE] Fetching released ONNX Runtime source v1.18.2"
git clone --depth=1 --branch v1.18.2 https://github.com/microsoft/onnxruntime.git "$root/onnxruntime"
cd "$root/onnxruntime"
echo "[PROBE] ORT revision $(git rev-parse HEAD)"

echo "[PROBE] Checking PS5 SDK iconv support"
test -f "$sdk/target/include/iconv.h" && echo "[PROBE] SDK iconv.h exists" || echo "[PROBE] SDK iconv.h absent; may require libiconv port"
echo "[PROBE] Configure with PS5 FreeBSD SDK, statically linked CPU backend"
cmake -S cmake -B "$root/build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$source_root/homebrew/bible-reader/native_piper/ps5-onnx-toolchain.cmake" \
  -DPS5_HOME="$sdk" \
  -Donnxruntime_CROSS_COMPILING=ON \
  -DIconv_IS_BUILT_IN=TRUE \
  -DIconv_INCLUDE_DIR="$sdk/target/include" \
  -Donnxruntime_BUILD_SHARED_LIB=OFF \
  -Donnxruntime_BUILD_UNIT_TESTS=OFF \
  -Donnxruntime_RUN_ONNX_TESTS=OFF \
  -Donnxruntime_DISABLE_CONTRIB_OPS=ON \
  -Donnxruntime_DISABLE_ML_OPS=ON \
  -Donnxruntime_USE_FULL_PROTOBUF=OFF \
  -Donnxruntime_USE_PREINSTALLED_EIGEN=ON \
  -Deigen_SOURCE_PATH=/usr/include/eigen3 \
  -Donnxruntime_ENABLE_PYTHON=OFF \
  -Donnxruntime_USE_MIMALLOC=OFF \
  -Donnxruntime_USE_XNNPACK=OFF \
  -Donnxruntime_USE_OPENMP=OFF \
  -Donnxruntime_MINIMAL_BUILD=OFF \
  -Donnxruntime_ENABLE_LTO=OFF \
  -DCMAKE_INSTALL_PREFIX="$root/install"
echo "[PROBE] Examining available ONNX static targets"
ninja -C "$root/build" -t targets all | grep -E 'onnxruntime_(session|providers|graph|framework|optimizer):|onnxruntime_(session|providers|graph|framework|optimizer)\.a' | head -40 || true
echo "[PROBE] Compiling native PS5 ONNX Runtime C API and CPU kernels"
cmake --build "$root/build" --target onnxruntime_session onnxruntime_providers onnxruntime_optimizer onnxruntime_framework onnxruntime_graph --parallel 4
echo "[PROBE] Verifying x86_64-sie-ps5 static library archive"
find "$root/build" -type f -name 'libonnxruntime*.a' -print
find "$root/build" -type f -name 'libonnxruntime*.a' | grep .
echo "[PROBE] Native ONNX Runtime library cross-compile successful"
