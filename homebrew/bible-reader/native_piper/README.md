# Native Piper for PS5 — development investigation

**Research only. Nothing is published to TESTBUILDS by this experiment.**
The shipped Bible Reader v0.1.6 uses a real Piper Thorsten neural **pre-rendered**
Genesis 1 preview, not Piper inference on PS5.

## Target architecture

1. Native PS5 CPU ONNX Runtime, statically linked (no glibc/Linux .so).
2. eSpeak NG phonemizer for the current language; convert UTF-8 phoneme
   codepoints through the *actual* model-specific Piper `phoneme_id_map`.
3. Feed the VITS/Thorsten model with `input` int64, `input_lengths` int64,
   `scales` float32, and optional `sid` for multi-speaker models.
4. Stream model float32 PCM output to PS5 SDL2 queue, respecting stop/cancel.
5. Maintain the existing continuous verse progression and karaoke word
   overlays. eSpeak event positions are **not exact timestamps for neural
   speech**; use model-aware alignment or measure word-chunk boundaries before
   claiming exact neural karaoke.
6. Keep Classic and pre-rendered Neural preview as automatic fallbacks.

## First build gate

Cross-compile a real ONNX Runtime CPU inference library for `x86_64-sie-ps5`
on GitHub Actions, including its transitive dependencies. FreeBSD ports have
packaged ORT for FreeBSD, but **PS5 SDK cross-compilation is not yet verified**.
A failure does not affect current app, version.json, or its release.

Run [Probe PS5 Native Piper ONNX Runtime](../../../.github/workflows/probe-bible-piper-ps5.yml)
on this branch / PR. Logs: `Piper-PS5-Native-ONNX-Probe-Logs`.

## Upstream and licence

ONNX Runtime is MIT licensed:
https://github.com/microsoft/onnxruntime/tree/v1.18.2 .
The existing German Thorsten Medium model is MIT on its distribution repo.
eSpeak NG retains GPL-3.0 and bundled language data/licences.
