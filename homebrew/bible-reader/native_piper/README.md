# Native Piper for PS5 — Bible Reader v0.1.7

German Piper Thorsten Medium neural audio is synthesized directly on the PS5
through a statically linked ONNX Runtime CPU engine. The experimental version
compiled in GitHub Actions and a hardware tester confirmed real verse playback.

This production pipeline retains the original PPSA99793 Bible Reader title.

## Build
1. Prepare SDK and cross-compile ONNX Runtime 1.18.2 for x86_64-sie-ps5.
2. Build static CPU session/provider libraries and dependencies (including
   RE2 and nsync), plus a PS5 thread-local destructor compatibility shim.
3. Generate the phoneme ID table from the pinned German Thorsten config.
4. Package the pinned ONNX model and offline eSpeak language data.
5. Link the native PS5 executable and verify complete ZIP contents.

The ONNX/MLAS patches are in patch_ps5_mlas.py and probe_native_onnx.sh;
no global AVX-512 flags are forced on the PS5 CPU.

## Voice behavior
- Neural HD (Deutsch): attempts on-device inference for all German verses.
- Automatic fallback: prerecorded Genesis 1 neural PCM, then classic eSpeak.
- English, French and Spanish: classic offline eSpeak NG.
- Karaoke alignment: *approximate*, based on rescaled eSpeak word timestamps.

## Known limits
ONNX inference runs synchronously for each verse and stop may wait for
the current verse to finish synthesizing. Long-session stability is not
proven yet. Logs live at /data/BibleReader/bible-reader.log.

Upstream: ONNX Runtime 1.18.2 (MIT), Thorsten voice and eSpeak NG 1.52.0
(GPL-3.0-or-later for eSpeak).
