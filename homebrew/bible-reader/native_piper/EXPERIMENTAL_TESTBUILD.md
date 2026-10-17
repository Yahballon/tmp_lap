# Native Piper experimental PS5 test ZIP

**Never publish this branch to TESTBUILD version.json automatically.**
The experimental workflow produces a downloadable GitHub Actions artifact
(`Bible-Reader-v0.1.7-NATIVE-PIPER-EXPERIMENTAL-PPSA99793`).

Native ONNX Runtime CPU static archives are linked with the PS5 homebrew app.
German Thorsten Medium .onnx is installed locally at
`assets/tts/neural/de/de_DE-thorsten-medium.onnx`. Voice Style > Neural HD
will try on-device inference first, then the existing pre-rendered Genesis 1
preview, then eSpeak. Other languages remain on eSpeak.

**This is NOT yet verified on real hardware.** Inspect
`/data/BibleReader/bible-reader.log` for PIPER_READY/PIPER_NATIVE or
PIPER_RUN_ERR/PIPER_FALLBACK. Inference may take significant CPU time and
stopping while synchronous ONNX runs may be delayed; do not use as a
daily-driver build.

Never merge/release automatically until compiled *and tested on PS5*.
