#!/usr/bin/env bash
set -euo pipefail
root="$(pwd)"
sdk="$root/.deps/native/ps5-payload-sdk"
test -d "$sdk/target/include"
source_root=/tmp/bible-espeak-ng-1.52.0
if [[ ! -d $source_root/.git ]]; then
    git clone --depth 1 --branch 1.52.0 https://github.com/espeak-ng/espeak-ng.git "$source_root"
fi
revision=$(git -C "$source_root" rev-parse HEAD)
[[ $revision == 4870adfa25b1a32b4361592f1be8a40337c58d6c ]] || {
    echo "Unexpected eSpeak NG upstream revision: $revision" >&2; exit 1;
}
cmake -S "$source_root" -B /tmp/bible-espeak-host \
    -DCMAKE_BUILD_TYPE=Release -DUSE_ASYNC=OFF -DUSE_MBROLA=OFF \
    -DUSE_SPEECHPLAYER=OFF -DUSE_LIBPCAUDIO=OFF -DUSE_LIBSONIC=OFF \
    -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF
cmake --build /tmp/bible-espeak-host --target data --parallel 4
for language in en de fr es ru; do test -s "/tmp/bible-espeak-host/espeak-ng-data/${language}_dict"; done

cat >/tmp/bible-espeak-ps5-toolchain.cmake <<EOF
set(CMAKE_SYSTEM_NAME FreeBSD)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER /usr/bin/clang-18)
set(CMAKE_C_COMPILER_TARGET x86_64-sie-ps5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_AR /usr/bin/llvm-ar-18)
set(CMAKE_RANLIB /usr/bin/llvm-ranlib-18)
set(CMAKE_C_FLAGS_INIT "-fvisibility-nodllstorageclass=default -isysroot ${sdk} -isystem ${sdk}/target/include -fno-stack-protector -fno-plt -femulated-tls -fno-omit-frame-pointer")
set(CMAKE_FIND_ROOT_PATH "${sdk}")
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
EOF
cmake -S "$source_root" -B /tmp/bible-espeak-cross \
    -DCMAKE_TOOLCHAIN_FILE=/tmp/bible-espeak-ps5-toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release -DUSE_ASYNC=OFF -DUSE_MBROLA=OFF \
    -DUSE_SPEECHPLAYER=OFF -DUSE_LIBPCAUDIO=OFF -DUSE_LIBSONIC=OFF \
    -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF
cmake --build /tmp/bible-espeak-cross --target espeak-ng --parallel 4
mkdir -p "$root/lib" "$root/src/third_party/espeak" "$root/assets/tts" "$root/assets/licenses"
cp "$source_root/COPYING" "$root/assets/licenses/eSpeakNG-GPL3.txt"
cp "$source_root/COPYING.UCD" "$root/assets/licenses/eSpeakNG-UnicodeData.txt"
cp -a "$source_root/src/include" "$root/src/third_party/espeak/include"
find /tmp/bible-espeak-cross -name libespeak-ng.a -exec cp {} "$root/lib/libespeak-ng.a" \;
find /tmp/bible-espeak-cross -name libucd.a -exec cp {} "$root/lib/libucd.a" \;
test -s "$root/lib/libespeak-ng.a"
test -s "$root/lib/libucd.a"
cp -a /tmp/bible-espeak-host/espeak-ng-data "$root/assets/tts/"
for language in en de fr es ru; do test -s "$root/assets/tts/espeak-ng-data/${language}_dict"; done
for profile in gmw/en gmw/de roa/fr roa/es zle/ru; do
    test -s "$root/assets/tts/espeak-ng-data/lang/$profile" || { echo "Missing voice profile $profile" >&2; exit 1; }
done
test -s "$root/assets/tts/espeak-ng-data/phondata"
test -s "$root/assets/tts/espeak-ng-data/phontab"
printf 'Offline TTS engine ready: %s bytes library and five languages\n' "$(stat -c %s "$root/lib/libespeak-ng.a")"
