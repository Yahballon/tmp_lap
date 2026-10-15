#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
base="$root/build/deps"
mkdir -p "$base"
cd "$base"
test -d PS5_Vulkan/.git || git clone -q https://github.com/mihawk-99/PS5_Vulkan.git
test -d PS5_PayloadSDK/.git || git clone -q https://github.com/mihawk-99/PS5_PayloadSDK.git
export PS5_PAYLOAD_SDK_FORK="$base/PS5_PayloadSDK"
vulkan="$base/PS5_Vulkan"
cd "$vulkan"
bash tools/setup-native-dependencies.sh
# PS5_Vulkan 'make' builds an unrelated demo app, NOT its driver archives.
# The prior build stopped on undefined symbol ps5_fp_ieee from that demo.
bash tools/rebuild-libc.sh
test -s runtime/libc.prx
# Rebuild the exact public shader compiler source tree from the signed SDK tag.
# The published v0.2.0 binary archive carries an SDK layout without the
# third_party source tree, so it cannot feed PS5_Vulkan's shader compiler.
sdk_checkout="$base/ps5-opengl-sdk-0.2.0"
if [[ ! -d "$sdk_checkout/.git" ]]; then
  git clone -q --depth=1 --branch sdk-0.2.0 \
    https://github.com/blackbearreloaded/ps5-opengl.git "$sdk_checkout"
fi
test "$(git -C "$sdk_checkout" rev-parse HEAD)" = bba419f0366528df9f16c73f44c661f7e1985cb7
(cd "$sdk_checkout" && python3 tools/fetch-sources.py)
export PS5_OPENGL_SDK="$sdk_checkout"
test -d "$PS5_OPENGL_SDK/third_party/opengnm-psbc"
test -f "$PS5_OPENGL_SDK/tools/fetch-sources.py"
echo "PS5 shader compiler SDK source checkout: $PS5_OPENGL_SDK"
bash tools/fetch-mesa.sh
bash tools/build-psbc-ps5.sh
bash tools/build-vulkan-runtime.sh
bash tools/build-driver.sh
for archive in \
  build/driver/ps5/libps5vk.ps5.a \
  .deps/native/vulkan-runtime/lib/libvk_runtime.ps5.a \
  build/driver/ps5/libpsbc_driver.ps5.a \
  .deps/native/psbc/lib/libpsbc_support.ps5.a; do
  test -s "$archive" || { echo "Missing native Vulkan archive $archive" >&2; exit 1; }
done
test -x .deps/native/ps5-payload-sdk/bin/prospero-clang
echo "PS5 Vulkan graphics archives ready for MesenCE title link"
