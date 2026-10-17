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
echo "[PROBE] Check Native Piper C API inference code with the real PS5 compiler"
clang++-18 --target=x86_64-sie-ps5 --sysroot="$sdk" \
  -isystem "$sdk/target/include/c++/v1" -isystem "$sdk/target/include" \
  -I"$root/onnxruntime/include/onnxruntime/core/session" \
  -I"$source_root/homebrew/bible-reader/native_piper" \
  -std=c++17 -fno-rtti -fvisibility-nodllstorageclass=default \
  -fno-stack-protector -fno-plt -femulated-tls -fsyntax-only \
  "$source_root/homebrew/bible-reader/native_piper/piper_onnx.cpp"
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
echo "[PROBE] Apply PS5 ABI thread-local max alignment compatibility patch"
python3 - <<'PY'
from pathlib import Path
file=Path('/tmp/piper-onnx-ps5/build/_deps/protobuf-src/src/google/protobuf/arena_impl.h')
s=file.read_text()
old='enum { kCacheAlignment = 64 };'
if old not in s:
    raise SystemExit('Unsupported protobuf source: expected cache alignment expression missing')
# PS5 clang target limits thread-local alignment to 32 bytes. Reduces
# cache-line padding, not the data type's semantic alignment requirements.
file.write_text(s.replace(old,'enum { kCacheAlignment = 32 };'))
print('PS5 protobuf TLS alignment adjusted: 64 -> 32')
PY
echo "[PROBE] Patch Linux-only CPU pinning / profiling and x86 TLS over-alignment for PS5"
python3 - <<'PY'
from pathlib import Path
root=Path('/tmp/piper-onnx-ps5/onnxruntime')
threadpool=root/'onnxruntime/core/common/threadpool.cc'
env=root/'onnxruntime/core/platform/posix/env.cc'
eigen=root/'include/onnxruntime/core/platform/EigenNonBlockingThreadPool.h'

def replace_checked(file,old,new,count):
    src=file.read_text()
    actual=src.count(old)
    if actual!=count:
        raise SystemExit(f'{file}: expected {count} occurrences, found {actual}: {old[:75]}')
    file.write_text(src.replace(old,new))
    print('PS5 portability patch:',file.name,'replaced',count)

replace_checked(threadpool,
  'core_ = sched_getcpu();',
  '// PS5 SDK exposes no sched_getcpu; CPU index is optional profiling data.\n  core_ = -1;',
  2)
replace_checked(env,
  '#if defined(CPUINFO_SUPPORTED) && !defined(__APPLE__) && !defined(__ANDROID__) && !defined(__wasm__) && !defined(_AIX)',
  '#if defined(CPUINFO_SUPPORTED) && !defined(__APPLE__) && !defined(__ANDROID__) && !defined(__wasm__) && !defined(_AIX) && !defined(BIBLE_ONNX_PS5)',
  1)
replace_checked(env,
  '#if !defined(__APPLE__) && !defined(__ANDROID__) && !defined(__wasm__) && !defined(_AIX)',
  '#if !defined(__APPLE__) && !defined(__ANDROID__) && !defined(__wasm__) && !defined(_AIX) && !defined(BIBLE_ONNX_PS5)',
  1)
replace_checked(eigen,
  '#if defined(__x86_64__)\n#define ORT_FALSE_SHARING_BYTES 128',
  '#if defined(BIBLE_ONNX_PS5)\n#define ORT_FALSE_SHARING_BYTES 32\n#elif defined(__x86_64__)\n#define ORT_FALSE_SHARING_BYTES 128',
  1)
PY
echo "[PROBE] Examining available ONNX static targets"
ninja -C "$root/build" -t targets all | grep -E 'onnxruntime_(session|providers|graph|framework|optimizer):|onnxruntime_(session|providers|graph|framework|optimizer)\.a' | head -40 || true
echo "[PROBE] Compiling native PS5 ONNX Runtime C API and CPU kernels"
cmake --build "$root/build" --target onnxruntime_session onnxruntime_providers onnxruntime_optimizer onnxruntime_framework onnxruntime_graph --parallel 4
echo "[PROBE] Building all available auxiliary CPU/static ONNX targets required by final executable linking"
ninja -C "$root/build" -t targets all > "$root/native-targets.txt"
for target in onnxruntime_common onnxruntime_util onnxruntime_mlas onnxruntime_flatbuffers onnxruntime_providers_cpu; do
  if grep -Eq "^lib${target}\\.a:" "$root/native-targets.txt"; then
    echo "[PROBE] Required additional target: $target"
    cmake --build "$root/build" --target "$target" --parallel 4
  else
    echo "[PROBE] No separate static target for $target"
  fi
done
echo "[PROBE] Verifying x86_64-sie-ps5 static library archive"
find "$root/build" -type f -name 'libonnxruntime*.a' -print
find "$root/build" -type f -name 'libonnxruntime*.a' | grep .
echo "[PROBE] Native ONNX Runtime library cross-compile successful"
