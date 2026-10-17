# Piper Neural HD PS5 Pilot (v0.1.6)

**Preview, not a native Piper inference port.** Microsoft ONNX Runtime does not
officially target FreeBSD/PS5. This pilot tests a substantially more natural
German voice on actual PS5 hardware without risking the working v0.1.5 TTS.

- Full offline native Bible Reader unchanged, eSpeak continues to read every
  language and every verse of the 66-book Bible.
- For **Deutsch > 1. Mose > Kapitel 1**, all 31 verses get real Piper Thorsten
  Medium neural speech, rendered using Piper on the CI build host, bundled as
  16-bit mono 22,050 Hz PCM files inside the same PS5 installation ZIP.
- Settings > Voice style: **Neural HD (Genesis 1 preview)** / Classic.
- When X starts reading German Genesis 1, the native audio worker streams the
  neural audio for each verse, then automatically falls back to eSpeak for
  chapters/verses without neural previews.
- Word highlights use eSpeak word-onset times scaled to the neural segment
  duration. **Approximate synchronization**, not phoneme-aligned word timing.
- Requires no account, cloud service, PC, or network on the PS5.
- Models are NOT transferred into the PS5 app: ONNX inference on-console is
  a separate portability milestone. Don't market this as full neural TTS.

Model upstream: `Thorsten-Voice/Piper`, 
`de_DE-thorsten-medium.onnx`, MIT model repository. Model SHA256:
`7e64762d8e5118bb578f2eea6207e1a35a8e0c30595010b666f983fc87bb7819`.
Original translation: Luther 1912 (public domain).
Engine: [Piper GPL-3.0](https://github.com/OHF-Voice/piper1-gpl)
(reproduction build only). The app does not bundle Piper Python or ONNX.
