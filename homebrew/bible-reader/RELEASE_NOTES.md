# Bible Reader v0.2.0 — Russian Bible, About and Unicode repair

- Adds **About** button at bottom of the sidebar, with developer **YAHBALLON**, version, credits and offline features.
- Adds **Русский** without replacing Stephen Hawking-inspired English Retro TTS: 66-book Russian Synodal Bible, public domain via eBible.org.
- Adds native offline neural speech using **Denis Medium** for Russian (CC0 voice-datasets source); Russian eSpeak Classic/fallback remains available.
- Fixes accented text displaying as '?' by normalizing the Bible corpus to NFC, embedding accented and Cyrillic glyphs in the PS5 SDF font, validating rendered codepoint coverage in CI, and adding Russian case-insensitive search and keyboard pages.
- Keeps all 4 previous neural voices, karaoke word highlighting, continuous reading, and persisted settings.
