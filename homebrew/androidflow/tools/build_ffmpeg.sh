#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
: "${PS5_PAYLOAD_SDK:?PS5_PAYLOAD_SDK is required}"
src=${FFMPEG_SRC:-"$root/.work/FFmpeg"}
out=${FFMPEG_BUILD:-"$root/.work/ffmpeg-build"}
prefix=${FFMPEG_PREFIX:-"$root/.work/ffmpeg-ps5"}
test -f "$src/configure" || { echo "Missing pinned FFmpeg source in $src" >&2; exit 2; }
mkdir -p "$out"
cd "$out"
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
