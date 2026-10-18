# Bible Reader v0.1.9 – Retro TTS exit and Neural HD restoration fix

- Fix being stuck in the robotic "Stephen Hawking (Retro TTS)" / Classic voice after switching language or restarting. The old version saved `voice_style=0` whenever Retro was selected, leaving regular voices in Classic.
- Switching from Retro to English, Deutsch, Français, or Español now restores **Neural HD** automatically, including when opening an old v0.1.8 saved settings file.
- Selecting Retro no longer overwrites the existing voice-style preference; Retro itself still uses the intentional monotone eSpeak preset.
- Reinitialize eSpeak pitch, rate, and range on **every** playback start to prevent retro voice settings from carrying over into normal Classic speech.
- Voice language and style changes are logged in `/data/BibleReader/bible-reader.log` to simplify future troubleshooting.
- Regression tests cover every Retro → normal language transition, normal-language Classic preservation, and restoration across simulated app restarts.
- Retains all four native offline Piper voices (Kristin / Thorsten / Siwis / DaveFX), existing karaoke highlighting, continuous playback and bookmarks.
