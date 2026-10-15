#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
base="$root/build/deps"
mkdir -p "$base"
cd "$base"
test -d PS5_Vulkan/.git || git clone https://github.com/mihawk-99/PS5_Vulkan.git
test -d PS5_PayloadSDK/.git || git clone https://github.com/mihawk-99/PS5_PayloadSDK.git
export PS5_PAYLOAD_SDK_FORK="$base/PS5_PayloadSDK"
cd PS5_Vulkan
bash tools/setup-native-dependencies.sh
if ! make > "$base/vulkan-build.log" 2>&1; then
  tail -n 120 "$base/vulkan-build.log" >&2
fi
for archive in build/driver/ps5/libps5vk.ps5.a .deps/native/vulkan-runtime/lib/libvk_runtime.ps5.a build/driver/ps5/libpsbc_driver.ps5.a .deps/native/psbc/lib/libpsbc_support.ps5.a; do
  test -s "$archive" || { echo "Missing Vulkan archive $archive" >&2; exit 1; }
done
test -s runtime/libc.prx
test -x .deps/native/ps5-payload-sdk/bin/prospero-clang
