# M11c – Native Godot AudioOut driver for PS5 v0.1.5 (EXPERIMENTAL)

The user's PS5 M11a hardware test created real 529,200-byte PCM and started AudioStreamPlayer but was SILENT. Godot OS_PS5 registered no real AudioDriver, leaving a dummy sink.

## Implementation

- Opt-in AudioDriverPS5 derives from original Godot 4.7.2 AudioDriver and registers with AudioDriverManager before Main::setup.
- A dedicated worker thread calls Godot audio_server_process for 256 frames at 48 kHz stereo. It converts 32-bit signed fixed-point mixer samples to interleaved 16-bit and submits to sceAudioOutOutput.
- The blocking AudioOut kernel call occurs only AFTER unlocking Godot's audio mixer mutex; there is no fake tone playback, audio file export or rumble workaround.
- The public PS5 payload SDK-generated libSceAudioOut.so stub is used for native imports; no proprietary Sony runtime binary is included.
- GODOT_PS5_AUDIOOUT=1 is opt-in for PPSA99847 v0.1.5 only. Known-good v0.1.4 physics, graphics, fonts and savegames remain the rollback build.

## Native CI

https://github.com/Yahballon/tmp_lap/actions/workflows/build-godot-ps5-m11c-native-audioout.yml

The workflow must validate a real compiled AudioDriverPS5, verify the public SDK audio import stub exists, link the real Godot engine with Physics2D + FreeType + GLES3, and only upload a signed installer ZIP if all package checks pass (PPSA99847 metadata content version 01.000.006). Passing host tests is NOT proof that audio is audible on console.

## PS5 acceptance

1. Install the signed v0.1.5 experimental test build when CI passes (retain v0.1.4 for rollback).
2. Keep /data/GodotPS5/games/m11-audio.pck and select it by putting m11-audio.pck in /data/GodotPS5/launch.txt.
3. Start the app, LISTEN for six notes and press X to replay. Real TV/headphone output must be heard to accept M11c.
4. Collect gles3-stdout.log, gles3-stderr.log and gles3-runtime.log, including driver registered/open/output diagnostics and any negative native AudioOut error codes.
5. Require nonnegative AudioOut handle, successful block writes, actual audible sound, clean GODOT_RETURN=0 and EGL_CLOSED. AUDIO_PLAYER_STARTED alone does not prove working sound.

Known probe ABI: sceAudioOutOpen user 0xff, port type 0, index 0, 256 frames, 48000 Hz, stereo signed 16-bit format 1. References: https://github.com/Rufidj/PS5_DEV_HOMEBREW/blob/main/ps5_sdk/ps5_sdk_audio_helpers.h and https://github.com/ps5-payload-dev/sdk/blob/master/sce_stubs/libSceAudioOut.c.

Status: M11c PS5 audibility remains OPEN pending native CI and real PS5 test. GitHub issue #247.