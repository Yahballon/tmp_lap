# PSJukebox v0.1.9 — Fix ZIP scan crash and clean coverflow (TESTBUILD)

- Fix native PS5 `SIGSEGV` during import of valid compressed `.album` containers, observed immediately after `ALBUM_EXTRACT_BEGIN` with archive `wu-tang-forever.album`.
- In `write_entry()`, compressed input/output scratch buffers were two 32 KiB local arrays on the scanner pthread stack. Now allocated on the heap so the scanner can read and extract albums without blowing its small native stack.
- Add a regression test that imports a packed album with cover, background and music on a deliberately constrained 64 KiB pthread stack. Keep the previous sandbox EPERM and helper tests.
- Remove the large album folder name rendered above the selected cover in the album carousel. Keep navigation, album counter, and now-playing track title unchanged.
- No changes to user music folders, .album contents, cached songs, SMB configuration, favorites or controller mapping.

No real-console confirmation until tested on PS5; if another issue occurs, send log lines from `BOOT v0.1.9`.
