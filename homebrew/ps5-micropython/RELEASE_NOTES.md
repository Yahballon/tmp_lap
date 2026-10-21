# PS5 MicroPython v0.2.1

Fixes user-provided Python files not appearing in the script list.

- Removed fragile `stat()` file-type and size gate; scan now tests whether each .py file can actually be read.
- Uses bounded streaming reads instead of `ftell` / `fseek` for user Python scripts.
- Shows live scan diagnostics in the library/console screen (accepted count, empty, oversized, unreadable and errno).
- Logs scan detail to `/data/PS5MicroPython/logs/scan.log` when log directory is writable.
- Distinguishes empty scripts (0 bytes) and oversized scripts (>64 KiB) from unreadable files.
- Added host regression tests with .py files, empty files, oversized files, symlinks and missing directories.
- Preserves embedded MicroPython, controller editor and existing user scripts.

Note: This is a filesystem detection and diagnostics fix. On-device visibility may still depend on the privileges/mount namespace of the native PS5 title. Report the displayed scan status if the file is still not listed.

Previous v0.2.0 release was built successfully; v0.2.1 requires a real PS5 rescan test.
