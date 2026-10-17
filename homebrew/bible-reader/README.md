# Bible Reader by YAHBALLON

Native PlayStation 5 offline Bible reader v0.1.0, designed from the provided three-column blue-and-gold reference screenshot. No network access is required at runtime.

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
- Release naming: `bible-reader-v0.1.0` (reserved for a verified future release)

## Building
The dedicated [GitHub Actions workflow](../../.github/workflows/build-bible-reader.yml) downloads and validates the public-domain Bible **at build time**, then assembles a PS5 native homebrew install ZIP with all 66 books. The release only exists after a successful build and explicit publication. You may also prepare `webpb.tsv` with:
`python3 tools/prepare_bible.py /tmp/webpb.tsv`.

The build uses the GPL-3.0-or-later [PS5-DevBridge](https://github.com/Yahballon/PS5-DevBridge) native scaffold (with its LGPL-compatible/other upstream notices as applicable). All UI artwork is procedural/original and uses no Sony/PlayStation logos. WEBPB Bible text is public domain: https://ebible.org/.

## Installing on PS5
Once Actions creates a verified `PPSA99793.zip`, extract the `PPSA99793/` folder to `/data/homebrew/PPSA99793/` and register/start the app with your compatible native homebrew loader. Do not confuse this with a Sony-installable PKG. Existing `/data/BibleReader/` settings persist across updates.

## Status
This repository branch contains the implementation and proposed build workflow. A native compile and hardware verification must be completed before a production release.
