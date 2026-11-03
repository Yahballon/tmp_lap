#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
: "${PS5_PAYLOAD_SDK:?PS5_PAYLOAD_SDK is required}"
src=${FFMPEG_SRC:-"$root/.work/FFmpeg"}
out=${FFMPEG_BUILD:-"$root/.work/ffmpeg-build"}
prefix=${FFMPEG_PREFIX:-"$root/.work/ffmpeg-ps5"}
test -f "$src/configure" || { echo "Missing pinned FFmpeg source in $src" >&2; exit 2; }
command -v nasm >/dev/null || { echo "FFmpeg optimized x86 decoders require NASM; install nasm" >&2; exit 2; }
nasm -v
mkdir -p "$out"
cd "$out"
# The previous build only printed "C compiler test failed". On failure keep
# the real compiler/linker diagnostic visible in Actions and as an artifact.
on_exit() {
  rc=$?
  if (( rc != 0 )); then
    echo "=== AndroidFlow FFmpeg cross-build failed (exit $rc) ===" >&2
    for diagnostic in "$out/ffbuild/config.log" "$out/config.log"; do
      if [[ -f "$diagnostic" ]]; then
        echo "=== $diagnostic (last 160 lines) ===" >&2
        tail -n 160 "$diagnostic" >&2
      fi
    done
  fi
}
trap on_exit EXIT
printf 'int main(void) { return 0; }\n' > "$out/compiler-check.c"
echo "Checking PS5 target linker before FFmpeg configure"
bash "$root/tools/ffmpeg-ps5-cc.sh" -v "$out/compiler-check.c" -o "$out/compiler-check.elf"
test -s "$out/compiler-check.elf"
echo "PS5 cross-link probe passed"
bash "$src/configure" --prefix="$prefix" \
  --enable-cross-compile --arch=x86_64 --target-os=freebsd \
  --cc="bash $root/tools/ffmpeg-ps5-cc.sh" --ld="bash $root/tools/ffmpeg-ps5-cc.sh" \
  --ar=llvm-ar-18 --ranlib=llvm-ranlib-18 --nm=llvm-nm-18 \
  --disable-autodetect --disable-everything --disable-programs --disable-doc \
  --disable-avdevice --disable-network --disable-hwaccels --disable-debug \
  --enable-avformat --enable-avcodec --enable-swscale --enable-swresample \
  --enable-protocol=file --enable-demuxer=mov,matroska,ogg,avi,mpegts \
  --enable-parser=h264,hevc,aac,vp8,vp9,opus \
  --enable-decoder=h264,hevc,aac,mp3,opus,vorbis,vp8,vp9,pcm_s16le \
  --enable-pic --enable-pthreads --disable-shared --enable-static
make -j"${BUILD_JOBS:-4}"
make install
for lib in avformat avcodec swscale swresample avutil; do
  test -s "$prefix/lib/lib$lib.a" || { echo "Missing lib$lib.a" >&2; exit 1; }
done
echo "Verified FFmpeg static PS5 archives in $prefix"
