#!/usr/bin/env bash
# FFmpeg PS5 cross C driver. Adapted from ProsperoEden GPL-3.0-or-later.
set -euo pipefail
: "${PS5_PAYLOAD_SDK:?PS5_PAYLOAD_SDK is required}"
args=()
link=true
for arg in "$@"; do
 case "$arg" in
  -c|-E|-S|-M|-MM|--version) link=false; args+=("$arg") ;;
  -lm|-lpthread|-pthread) ;;
  *) args+=("$arg") ;;
 esac
done
if "$link"; then
  # Follow the working ProsperoEden PS5 FFmpeg linker recipe; the target
  # toolchain's bin directory supplies the linker and PS5-only tools.
  export PATH="$PS5_PAYLOAD_SDK/bin:$PATH"
  args+=(-nostdlib -Wl,-e,main -Wl,--no-undefined
    -L "$PS5_PAYLOAD_SDK/target/lib" -lSceLibcInternal -lkernel -lc -lSceNet)
fi
exec clang-18 --target=x86_64-sie-ps5 -isysroot "$PS5_PAYLOAD_SDK" \
  -isystem "$PS5_PAYLOAD_SDK/target/include" \
  -fvisibility-nodllstorageclass=default -fno-stack-protector -fno-plt \
  -femulated-tls -ffunction-sections -fdata-sections "${args[@]}"
