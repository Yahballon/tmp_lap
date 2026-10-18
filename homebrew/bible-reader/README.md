# Bible Reader by YAHBALLON

Native PlayStation 5 offline Bible reader v0.2.0, designed from the provided three-column blue-and-gold reference screenshot. No network access is required at runtime.

## Features
- 66 books / 1189 chapters of the public-domain World English Bible British Edition (WEBPB), shipped as plain verse data in the installation ZIP
- Old and New Testament picker, chapter chooser and verse-by-verse reading with scrolling
- Offline phrase search with on-screen DualSense character picker; up to 150 matches
- Persistent bookmarks, text size, theme, daily verse toggle and last reading position
- Daily verse, previous/next chapter, safe exit confirmation, selection highlighting
- Direct native C++ OpenGL app using the current PS5-DevBridge homebrew scaffold

## Controller
D-pad: Navigate; **X**: Select/Open; **O**: Back (exit confirmation from the main menu); **SQUARE**: Toggle bookmark/delete search character; **TRIANGLE**: Search; **R2**: Run search while entering text; **L1/R1**: Previous/next chapter; **L2**: Open daily verse; **OPTIONS**: Settings.

## Files
- Title ID: `PPSA99793`
- Persistent preferences: `/data/BibleReader/settings.txt`
- Offline Bible dataset: `assets/bible/webpb.tsv`
- Release naming: `bible-reader-v0.2.0`

## Building
The dedicated [GitHub Actions workflow](../../.github/workflows/build-bible-reader.yml) downloads and validates the public-domain Bible **at build time**, then assembles a PS5 native homebrew install ZIP with all 66 books. The release only exists after a successful build and explicit publication. You may also prepare `webpb.tsv` with:
`python3 tools/prepare_bible.py /tmp/webpb.tsv`.

The build uses the GPL-3.0-or-later [PS5-DevBridge](https://github.com/Yahballon/PS5-DevBridge) native scaffold (with its LGPL-compatible/other upstream notices as applicable). All UI artwork is procedural/original and uses no Sony/PlayStation logos. WEBPB Bible text is public domain: https://ebible.org/.

## Installing on PS5
Once Actions creates a verified `PPSA99793.zip`, extract the `PPSA99793/` folder to `/data/homebrew/PPSA99793/` and register/start the app with your compatible native homebrew loader. Do not confuse this with a Sony-installable PKG. Existing `/data/BibleReader/` settings persist across updates.

## Status
The GitHub Actions workflow compiles, verifies and publishes immutable installable ZIPs on successful main-branch builds. Native German Piper output was tested successfully on PS5 hardware in the experimental LAB. Long-session stability and stop latency require more testing.

## v0.1.6 formatting fix
- Converts unsupported WEBPB curly quotes, typographic apostrophes, dashes and Unicode spaces into equivalent glyphs supported by the native UI font at build time (for example `God's` and `"Let there be light"`).
- Daily verse heading uses a native drawn sun rather than an unsupported Unicode symbol.
- Variable-height Bible verses now auto-scroll the list to keep the highlighted verse inside the reading viewport; long verse texts can be scrolled line by line with D-pad Up/Down.
- Existing bookmarks, font settings and reading position are preserved. Re-run the full PS5 build when updating the text corpus.

## v0.1.6 responsive reading layout
- Settings > Daily verse panel = **On**: retains the original three-column design (the centre reader is 927px wide).
- Settings > Daily verse panel = **Off**: hides the entire Daily Verse box, not just its text, and grows the centre reading panel to 1416px while keeping the sidebar and margins unchanged.
- Bible text reflows to 1219px instead of 730px; automatic selected-verse scrolling uses the same current width and stays inside the reading area.
- Search results can use the additional space too. Changes apply immediately after toggling the setting, persist across restarts, and do not reset reading position or bookmarks.

## v0.1.6 complete offline internationalisation
- Language picker in Settings: **English, Deutsch, Français, Español**.
- Every language includes its own complete, licensed public-domain 66-book text, not machine-translated English verses:
  English [WEBPB](https://ebible.org/details.php?id=engwebpb);
  German [Lutherbibel 1912](https://ebible.org/details.php?id=deu1912);
  French [Louis Segond 1910](https://ebible.org/details.php?id=fraLSG);
  Spanish [Reina-Valera 1909](https://ebible.org/details.php?id=spaRV1909).
- All 66 book names, menu navigation, settings, daily verses and offline search are translated.
- Font atlas is regenerated at build time with Unicode Latin coverage for German umlauts, French and Spanish accents (DejaVu Sans derivative; see included licence).
- Language persists in `/data/BibleReader/settings.txt`; old settings files default to English.
- All languages and extended glyphs are bundled in the installation ZIP. Switching languages never needs Internet access.
- Brand subtitle displays `by YAHBALLON   v0.1.6` together instead of at upper-right.

## v0.1.6 Offline TTS & Karaoke
- Press X on a verse to **speak it immediately** and automatically continue through every subsequent verse, chapter and book in the active language.
- Press O to stop. Opening Settings, changing chapters or using Search safely closes the TTS worker.
- The live word's glyphs are overpainted in **karaoke yellow**, synchronized with eSpeak NG word-event audio timestamps and SDL queued-audio playback, not a fake timer.
- Four offline voices: English, German, French, Spanish. eSpeak NG 1.52.0 is pinned, statically compiled from upstream on CI and its language data bundled. Runtime does not connect to a server.
- A thread handles audio so the UI stays responsive. Long verses auto-follow the spoken line.
- Spoken voices are synthesized (eSpeak), not photorealistic. Real PS5 hardware verification is needed for accurate A/V sync and sound output.
- Upstream source: https://github.com/espeak-ng/espeak-ng/tree/1.52.0 (GPL-3.0-or-later). SDL2 is provided by PacBrew; relevant license files are in the ZIP.

## v0.1.6 - voice selection fix and persistent troubleshooting log
- Fix eSpeak NG voice resolution: select via locale properties (`en/de/fr/es`) and fall back to the **canonical profile identifier** (`gmw/en`, `gmw/de`, `roa/fr`, `roa/es`).
- Check installed voice profile, phondata and dictionary are readable before initializing synthesizer.
- Disable eSpeak process termination on voice initialization errors; log error codes instead.
- Save full TTS diagnostics on the PS5 to **`/data/BibleReader/bible-reader.log`**. Read via PS5 FTP/file browser after testing. A 1 MiB rotating `bible-reader.previous.log` holds prior diagnostics.
- Log application startup, selected language, source paths, all discovered TTS voices, voice-selection result, SDL audio status and synthesis errors. No verse texts, seeds or personal data are logged.
- Verify packaged eSpeak language profiles and the ability to actually produce speech samples in all four languages on the CI Linux host.
- PS5 hardware output still needs validation after this fix.

## v0.1.6 Piper Neural HD pilot
See [PIPER_HD_PILOT.md](./PIPER_HD_PILOT.md). This version packs real neural audio for German Genesis chapter 1, but **does not yet run the ONNX network on the PS5**. Select Settings > Voice style > Neural HD (1. Mose 1). Only this chapter uses higher-quality pre-rendered speech. The rest of the Bible and other languages remain classic eSpeak offline. Karaoke synchronization in this preview is approximate. All existing v0.1.5 functionality is preserved.

## v0.2.0 Native Piper — offline PS5 neural speech

- **German Lutherbibel 1912:** Native Thorsten Medium synthesis using ONNX Runtime CPU on the PS5 for arbitrary verses, not just Genesis 1. Model and dependencies are bundled in the installation ZIP. No Internet required on the console.
- Settings > Voice style > **Neural HD (Deutsch)** to use the native voice. English, French and Spanish remain offline classic eSpeak NG.
- **Continuous speech** across verses, chapters and books; keeps live yellow karaoke highlighting (word offsets approximated by rescaling eSpeak timings to Piper PCM length, not forced alignment).
- If model initialization or native synthesis fails, automatically uses the pre-rendered German Genesis 1 preview where available or classic eSpeak otherwise.
- Diagnostics are written to `/data/BibleReader/bible-reader.log`; check `PIPER_READY`, `PIPER_NATIVE`, `PIPER_RUN_ERR` and `PIPER_FALLBACK`.
- Caveat: ONNX inference is still synchronous per verse, so stopping during synthesis may incur a delay; heavy/endurance use is not fully validated. The native model adds download size and RAM use.
- Pinned ONNX Runtime 1.18.2 (MIT), Piper Thorsten model and eSpeak NG 1.52.0 (GPL); build sources are under `native_piper/`.

## v0.2.0 — Four-language Neural HD and a retro TTS voice preset

- Native offline Piper for English (Kristin Medium), German (Thorsten Medium), French (Siwis Medium) and Spanish (DaveFX Medium). The PS5 uses the selected voice's model-specific eSpeak IPA phoneme map; no external runtime service.
- **Settings > Language > Stephen Hawking (Retro TTS)** is an extra playful computer-voice preset reading the *English* Bible with original eSpeak synthesis (rate 145, pitch 36, range 1). It is not an imitation or recording of his unique DECtalk speech voice.
- Yellow karaoke highlighting persists with approximate native Piper time rescaling; eSpeak classic timing is exact.
- Models are packaged offline and verified with SHA256 at CI build time. Model cards and dataset credits are included. Expect a much larger installer than v0.1.7.
- Continuous reading, bookmarks and fallback to the last known-good eSpeak system are retained.

## v0.2.0 – Fix Retro speech remaining active
If the computer-like Retro voice was selected, v0.1.8 overwrote the saved voice style with Classic, causing the other languages to keep sounding robotic even after a restart. Selecting any normal language from Retro now restores Neural HD automatically, and eSpeak's rate, pitch and range are explicitly returned to standard values on each normal speech start. The old preference file and bookmarks remain intact.

## v0.2.0: About, Russian and UTF-8 glyph repair
- New **About** page at bottom of left sidebar: author YAHBALLON, offline features, credits and version.
- New **Русский** entry (after the Retro English preset): full public-domain Russian Synodal Bible (1876), Cyrillic navigation and an on-screen Cyrillic search keyboard with pagination.
- **Neural HD Russian** uses Denis Medium (CC0 voice-datasets model), generated from the actual Piper configuration using the existing PS5 ONNX Runtime. Russian eSpeak NG remains available in Classic mode and as a fallback.
- Fixes font atlas missing-glyph behavior with NFC-normalized corpus text, baked German ÄäÖöÜüß and Cyrillic glyphs, and explicit CI assertions that fonts/bundled verses contain their original Unicode.
- All five existing settings options plus the Stephen Hawking-inspired **Retro TTS** remain unchanged. Voice attribution is bundled with the homebrew ZIP.
