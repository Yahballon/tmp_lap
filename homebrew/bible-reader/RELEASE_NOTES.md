# Bible Reader v0.1.8 - Native Piper in four languages and retro TTS mode

- German: Thorsten Medium; English: Kristin Medium; French: Siwis Medium; Spanish: DaveFX Medium.
- Four bundled ONNX Piper models are executed fully offline on the PS5, not prerecorded audio.
- Native selection automatically follows Settings > Language, with the correct eSpeak phonemes and model-specific phoneme-to-ID tables. Voice style > Neural HD uses Piper and falls back to classic eSpeak if a voice cannot be loaded.
- Settings > Language includes **Stephen Hawking (Retro TTS)** as a playful fifth mode. Uses original eSpeak software tuned to a monotone, computer-like timbre reading the full English WEBPB Bible, not an audio recording or clone of Stephen Hawking's actual voice.
- Existing yellow karaoke highlighting remains, with approximate timing adjustments based on the native audio length; bookmarks and continuous reading preserved.
- Additional model files enlarge the application size considerably. All three voice models are SHA256 validated during CI.
- Model cards included in the installed files under assets/licenses for voice-specific license/attribution.
- This is a hardware-test candidate; validate CPU performance, audio quality, and memory on your PS5 before considering a stable release.
