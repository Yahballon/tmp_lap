# PS5DreamcastX v0.1.9

## All games visible on startup

- Fixed initial USB library selection: the catalogue now opens **All games**
  instead of **Recently played** when you have launched games before.
- With 5 scanned Dreamcast titles and 2 recent titles, the shelf now starts
  at **5/5**, rather than mistakenly looking like a two-game library.
- Keeps Recently played and Favourites accessible using up/down on the D-pad.
- Incremental rescans and source-tab changes retain the complete library
  as the initial shelf; disc-selection retains its disc-specific priority.
- Added C++ regression checks for startup, partial rescans, no recents and
  the multi-disc selection dialog.
- No changes to GPU cover rendering, local artwork cache, ROM discovery,
  BIOS, save files or controller controls.

PS5 install ZIP: `PPSA99765.zip`; Title ID: `PPSA99765`.
