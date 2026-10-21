# PS5 MicroPython v0.2.0

## Added

- Built-in DualSense-operated Python text editor, accessible with Options from the main script library.
- New scripts saved as automatically numbered .py files in /data/PS5MicroPython/scripts/.
- Edit existing user scripts or create a user-owned copy of a packaged demo.
- On-screen keyboards for lowercase, uppercase, symbols, newline and four-space indent.
- Text caret navigation and backspace; confirm before discarding unsaved changes.
- Atomic save with temporary file and rename; validates path, filename and 64 KiB file limit.
- Dedicated host regression tests for editor text manipulation and saved files.

## Unchanged

- MicroPython v1.26.1 embedded VM, previous sample scripts and persistent user script directory.

## Known limits

PS5 on-console editor/save validation remains pending. Script execution blocks the UI until completion and MicroPython uses a minimal runtime. Prior PS5 user test verified v0.1.0 Fibonacci output.
