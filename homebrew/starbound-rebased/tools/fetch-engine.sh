#!/usr/bin/env bash
set -euo pipefail
revision=609c5af217abf6eb55d3eb594df4db76cfb0c90c
target="${1:-build/gold872-engine}"
git clone --quiet --filter=blob:none --no-checkout https://github.com/Gold872/Super-Mario-Bros.git "$target"
git -C "$target" sparse-checkout init --no-cone
printf '/*\n/src/\n/include/\n/LICENSE\n' | git -C "$target" sparse-checkout set --stdin
git -C "$target" checkout --quiet "$revision"
test "$(git -C "$target" rev-parse HEAD)" = "$revision"
test -f "$target/src/main.cpp"
test -f "$target/LICENSE"
if test -d "$target/res"; then
  echo "FAIL: upstream artwork must not be fetched" >&2
  exit 1
fi
echo "Fetched original MIT C++ engine without proprietary media"
