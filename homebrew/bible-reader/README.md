# Bible Reader by YAHBALLON

Native PlayStation 5 offline Bible reader v0.1.2, designed from the provided three-column blue-and-gold reference screenshot. No network access is required at runtime.

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
- Release naming: `bible-reader-v0.1.2`

## Building
The dedicated [GitHub Actions workflow](../../.github/workflows/build-bible-reader.yml) downloads and validates the public-domain Bible **at build time**, then assembles a PS5 native homebrew install ZIP with all 66 books. The release only exists after a successful build and explicit publication. You may also prepare `webpb.tsv` with:
`python3 tools/prepare_bible.py /tmp/webpb.tsv`.

The build uses the GPL-3.0-or-later [PS5-DevBridge](https://github.com/Yahballon/PS5-DevBridge) native scaffold (with its LGPL-compatible/other upstream notices as applicable). All UI artwork is procedural/original and uses no Sony/PlayStation logos. WEBPB Bible text is public domain: https://ebible.org/.

## Installing on PS5
Once Actions creates a verified `PPSA99793.zip`, extract the `PPSA99793/` folder to `/data/homebrew/PPSA99793/` and register/start the app with your compatible native homebrew loader. Do not confuse this with a Sony-installable PKG. Existing `/data/BibleReader/` settings persist across updates.

## Status
The GitHub Actions workflow compiles, verifies and publishes immutable installable ZIPs on successful main-branch builds. Hardware verification of v0.1.2 is still needed.

## v0.1.2 formatting fix
- Converts unsupported WEBPB curly quotes, typographic apostrophes, dashes and Unicode spaces into equivalent glyphs supported by the native UI font at build time (for example `God's` and `"Let there be light"`).
- Daily verse heading uses a native drawn sun rather than an unsupported Unicode symbol.
- Variable-height Bible verses now auto-scroll the list to keep the highlighted verse inside the reading viewport; long verse texts can be scrolled line by line with D-pad Up/Down.
- Existing bookmarks, font settings and reading position are preserved. Re-run the full PS5 build when updating the text corpus.

## v0.1.2 responsive reading layout
- Settings > Daily verse panel = **On**: retains the original three-column design (the centre reader is 927px wide).
- Settings > Daily verse panel = **Off**: hides the entire Daily Verse box, not just its text, and grows the centre reading panel to 1416px while keeping the sidebar and margins unchanged.
- Bible text reflows to 1219px instead of 730px; automatic selected-verse scrolling uses the same current width and stays inside the reading area.
- Search results can use the additional space too. Changes apply immediately after toggling the setting, persist across restarts, and do not reset reading position or bookmarks.
