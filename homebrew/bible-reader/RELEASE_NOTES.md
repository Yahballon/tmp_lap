# Bible Reader v0.2.1 — Russian voice repair

- Fixes the PS5 Russian reading error **Voice unavailable (see BibleReader log)** reported on v0.2.0.
- Corrects an out-of-bounds eSpeak fallback profile table: the fifth native-language slot now resolves to `zle/ru` instead of an unrelated file path.
- Centralizes language-to-eSpeak profile mapping so asset validation and actual voice selection always use identical profile names.
- Adds a regression test for all five classic/fallback voices and an explicit eSpeak `SetVoiceByName` smoke test with the bundled offline assets.
- Keeps native Piper Denis Medium, five-language neural synthesis, Retro TTS, karaoke word highlighting, full offline Bible, and other language support unchanged.
