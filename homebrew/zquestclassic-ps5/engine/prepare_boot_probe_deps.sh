#!/usr/bin/env bash
# PS5 engine diagnostic tool bootstrap.
# Verify unchanged public PS5 payload SDK v0.42 by SHA-256. Use the distro's
# trusted host zlib1g-dev *only* for host ps5-native-tool; NEVER for target PS5.
set -euo pipefail
root="${GITHUB_WORKSPACE:?}/app"
cache="$root/.deps/native"
mkdir -p "$cache"
sdk="$cache/ps5-payload-sdk"
archive="$cache/ps5-payload-sdk.zip"
sdk_url="https://github.com/ps5-payload-dev/sdk/releases/download/v0.42/ps5-payload-sdk.zip"
sdk_hash="8cfbc7cd5811e719eb4f0c47eea668d3dc7b40bc8ab11c4a5031d40c23ec02da"
if [[ ! -x "$sdk/bin/prospero-lld" ]]; then
  curl -fL --retry 5 --retry-all-errors --connect-timeout 30 "$sdk_url" -o "$archive"
  printf '%s  %s\n' "$sdk_hash" "$archive" | sha256sum -c --strict
  unzip -q -o "$archive" -d "$cache"
fi
test -x "$sdk/bin/prospero-lld"
test -f "$sdk/target/include/c++/v1/vector"
test -f "$sdk/target/lib/libScePad.so"

host="$cache/zlib/root"
mkdir -p "$host/usr/include" "$host/usr/lib"
host_library="$(gcc -print-file-name=libz.a)"
test -f "$host_library" || { echo "Missing host libz.a: apt install zlib1g-dev" >&2; exit 5; }
test -f /usr/include/zlib.h
test -f /usr/include/x86_64-linux-gnu/zconf.h
cp /usr/include/zlib.h /usr/include/x86_64-linux-gnu/zconf.h "$host/usr/include/"
cp "$host_library" "$host/usr/lib/libz.a"
printf 'Using verified PS5 SDK v0.42, Ubuntu host libz.a for packaging tools only\n'

# The existing PS5-DevBridge OpenGL preparation calls an unrelated source
# downloader even when SDK + host zlib are present. Both are verified here.
python3 - "$root/tools/prepare-opengl.sh" <<'PY'
from pathlib import Path
import sys
p=Path(sys.argv[1]); text=p.read_text()
old='bash "$root/tools/setup-native-dependencies.sh" >/dev/null'
assert text.count(old)==1, "PS5-DevBridge OpenGL bootstrap changed"
text=text.replace(old,'test -x "$root/.deps/native/ps5-payload-sdk/bin/prospero-lld"')
p.write_text(text)
PY
