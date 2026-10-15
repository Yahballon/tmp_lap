#!/usr/bin/env bash
# Reproduce the PSFlycast v1.1.0 RADV PS5 toolchain in this app's build dir.
set -euo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
work="$root/build"
mkdir -p "$work"
cd "$work"
clone_if_missing() {
  if [[ ! -d "$1/.git" ]]; then git clone "$2" "$1"; fi
}
clone_if_missing PS5_VulkanTemplate https://github.com/mihawk-99/PS5_VulkanTemplate.git
clone_if_missing PS5_Vulkan https://github.com/mihawk-99/PS5_Vulkan.git
clone_if_missing PS5_Mesa https://github.com/mihawk-99/PS5_Mesa.git
clone_if_missing PS5_PayloadSDK https://github.com/mihawk-99/PS5_PayloadSDK.git
clone_if_missing libsmb2 https://github.com/sahlberg/libsmb2.git
git -C PS5_Vulkan checkout --detach 5b5e4fc2d80fdb67a4f61ba9e6a8a424026bb8a5
git -C libsmb2 checkout --detach 7e4ff97cf00edb288b6b763888e4326842afdc76
if [[ ! -f PS5_Mesa/.dreamcastx-patched ]]; then
  git -C PS5_Mesa checkout -B dreamcastx-patches 7b59ef27c1b09b9671bc4153c41940c3155c3af2
  git -C PS5_Mesa am "$work/flycast/shell/ps5/mesa/"*.patch
  touch PS5_Mesa/.dreamcastx-patched
fi
sed -i "s/^mesa_revision=.*/mesa_revision=$(git -C PS5_Mesa rev-parse HEAD)/" PS5_Vulkan/tools/build-radv.sh
export PS5_VULKAN_DIR="$work/PS5_Vulkan"
export PS5_MESA_FORK="$work/PS5_Mesa"
export PS5_PAYLOAD_SDK_FORK="$work/PS5_PayloadSDK"
# The upstream script can finish with a nonzero code solely because PS5_HOST is
# not configured; the workflow only needs the toolchain and build libraries.
set +e
bash PS5_VulkanTemplate/ps5/tools/bootstrap.sh
status=$?
set -e
if (( status != 0 )); then
  for p in PS5_Vulkan/.deps/native/radv-release/lib/libvulkan_radeon.ps5.a PS5_Vulkan/runtime/libc.prx PS5_Vulkan/build/host/ps5-native-tool; do
    test -s "$p" || { echo "Bootstrap failed: $p is missing" >&2; exit "$status"; }
  done
fi
sdk=b5efad528e0ac1b8289f39d72a0e890ff68ff0a6
if [[ ! -x sdk-dreamcastx/ps5-payload-sdk/bin/prospero-clang++ ]]; then
  mkdir -p sdk-dreamcastx sdk-tree
  git -C PS5_PayloadSDK archive "$sdk" | tar -x -C sdk-tree
  bash sdk-tree/platform/tools/setup-sdk.sh "$work/sdk-dreamcastx/ps5-payload-sdk" "$sdk" "$work/sdk-dreamcastx"
fi
test -x sdk-dreamcastx/ps5-payload-sdk/bin/prospero-clang++
echo "PS5 native dependencies ready"
