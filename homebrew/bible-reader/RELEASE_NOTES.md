# Bible Reader v0.1.5 - Offline voice fix + PS5 diagnostics

- Fix `Voice unavailable`: resolve offline eSpeak NG voices using their language codes and real profile IDs.
- Confirm phoneme assets, dictionaries and selected language profiles exist before initializing speech.
- Persistent TTS diagnostic log: `/data/BibleReader/bible-reader.log` (rotates at 1 MiB).
- Log PS5 startup, voice enumeration, TTS initialization, chosen voice, SDL audio backend, and audio synthesis errors.
- Add host integration checks for real spoken PCM in English, German, French and Spanish.
- Keep existing karaoke highlighting, continuous playback, offline Bible translations, existing settings and Title ID.
