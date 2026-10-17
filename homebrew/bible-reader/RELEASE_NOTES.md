# Bible Reader v0.1.6 - German Piper Neural HD Preview

- New Voice style setting: Classic / Neural HD (1. Mose 1).
- Genuine offline Piper Thorsten Medium neural speech for all 31 verses of German Genesis chapter 1.
- Speech rendered at GitHub Actions build time using a pinned real neural model, then delivered as PCM with the install ZIP.
- Continuous reading switches automatically to eSpeak for chapters without Neural HD previews.
- Yellow karaoke still present with approximate word-onset alignment in preview.
- eSpeak voice fallback for English, French, Spanish and the full Bible remains fully functional.
- Logs neural sample usage and fallback in /data/BibleReader/bible-reader.log.
- This preview is **not native on-console Piper ONNX synthesis**. That requires an unsupported PS5/FreeBSD ONNX runtime port and hardware validation.
