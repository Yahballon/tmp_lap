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
# The Vulkan ps5vk shader compiler requires the older PS5 OpenGL SDK layout.
# Use a pinned public release archive, verifying the upstream checksum.
sdk_tar="$base/ps5-opengl-sdk-0.2.0.tar.gz"
sdk_sha="$sdk_tar.sha256"
if [[ ! -f "$sdk_tar" ]]; then
  curl -fL --retry 4 --retry-delay 4 \
    "https://github.com/blackbearreloaded/ps5-opengl/releases/download/sdk-0.2.0/ps5-opengl-sdk-0.2.0-1440p120-sdl2.tar.gz" -o "$sdk_tar"
  curl -fL --retry 4 --retry-delay 4 \
    "https://github.com/blackbearreloaded/ps5-opengl/releases/download/sdk-0.2.0/ps5-opengl-sdk-0.2.0-1440p120-sdl2.tar.gz.sha256" -o "$sdk_sha"
fi
# Checksum manifest uses the release asset's filename. Reconstruct its expected
# digest check independent of the chosen local archive basename.
checksum=$(awk '{print $1}' "$sdk_sha" | head -1)
[[ $checksum =~ ^[0-9a-fA-F]{64}$ ]] || { echo "Invalid OpenGL SDK SHA-256" >&2; exit 1; }
printf '%s  %s\n' "$checksum" "$sdk_tar" | sha256sum -c -
sdk_root="$base/sdk-opengl-v0.2"
if [[ ! -f "$sdk_root/.unpacked" ]]; then
  mkdir -p "$sdk_root"
  tar -xzf "$sdk_tar" -C "$sdk_root"
  touch "$sdk_root/.unpacked"
fi
sdk_find=$(find "$sdk_root" -type d -path '*/third_party/opengnm-psbc' -print -quit)
if [[ -z $sdk_find ]]; then
  echo "The pinned OpenGL SDK archive has no third_party/opengnm-psbc source layout" >&2
  find "$sdk_root" -maxdepth 3 -type d | head -60 >&2
  exit 1
fi
export PS5_OPENGL_SDK="${sdk_find%/third_party/opengnm-psbc}"
test -f "$PS5_OPENGL_SDK/dependencies.json"
test -f "$PS5_OPENGL_SDK/tools/fetch-sources.py"
echo "PS5 shader compiler SDK: $PS5_OPENGL_SDK"
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
