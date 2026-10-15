# Ebook Reader v0.1.4 — USB bridge support

Native PS5 EPUB/PDF text reader. Title ID: `PPSA99741`.

## What's new

The ebook reader now uses the **same working USB access pattern as Payload LauncherX**. When the PS5 app sandbox denies `/mnt/usb0/Books` with `Operation not permitted`, it launches the bundled read-only `bookscan.elf` through an already-running compatible `elfldr` on loopback TCP port **9021**. The helper runs separately, scans books on USB and exposes a restricted catalogue/selected-file transfer service to the reader on loopback TCP port **9175**.

The application does **not** change the reader process's privileges, patch the kernel, or write to USB storage. Helper requests are limited to regular EPUB/PDF files (128 MiB max each) underneath the configured USB Books folder; symlinks and traversal are rejected.

## USB books

1. Put your ebook files under `/mnt/usb0/Books/`, including nested author subfolders.
2. Have a compatible PS5 `elfldr` already running on `127.0.0.1:9021`, just as with Payload LauncherX.
3. Install the complete PS5 folder including `bookscan.elf`. The file must sit alongside `eboot.bin`.
4. Start Ebook Reader. On startup it loads its local library and requests the USB catalogue through the helper. Press **OPTIONS** to rescan after adding books. Books detected via the helper appear in the normal bookshelf.
5. Press **X** on a USB book. The app downloads that one file over the local bridge to its own writable cache under `/download0/ebook-reader-cache` (or the data-library cache fallback), then reads it. **The original USB file remains untouched.**

The **TRIANGLE** storage panel still displays direct sandbox paths for diagnostics. An EPERM result for those paths can coexist with a functioning separate USB helper. The helper status is displayed independently at the bottom.

## Alternative file storage

For environments without elfldr, add legally obtained EPUB/PDF files to the installed `/data/homebrew/PPSA99741/assets/Books/` directory through an authorized PS5 FTP/file manager, then rescan. The app also checks `/download0/Books` and `/download0/ebooks` where available. Preserve your personal books and saved data when updating the app.

## Features

- Wood-themed native TV bookshelf, collection tabs, list/grid mode and DualSense controls
- Native EPUB chapter text extraction and simple PDF extractable-text preview
- Favourite books, persisted reading progress, local file import and storage diagnostics
- Separate, bounded, read-only USB helper modelled on Payload LauncherX
- Full GitHub Actions executable/package tests, versioned release and TESTBUILDS catalog entry

## Known limitations

Requires a compatible already-running elfldr for accessing an inaccessible USB mount. The USB helper runs outside the reader sandbox, so only use a trusted loader/runtime. Firmware-specific helper execution must be tested on the real console: passing CI alone is not hardware confirmation. DRM and EPUB imagery/CSS are unsupported; PDF is text preview only. Readable book file limit: 128 MiB. FTP fallback requires an authorized file manager.

## Building

Dedicated workflow: [build-ebook-reader.yml](../../.github/workflows/build-ebook-reader.yml). Uses PS5-DevBridge, OpenGL SDK, zlib and its native payload SDK to compile `bookscan.elf`; both `eboot.bin` and `bookscan.elf` are verified in the installable `PPSA99741.zip`.

Release tag: `ebook-reader-v0.1.4`.

Source license: GPL-3.0-or-later, respecting dependency licenses.

## v0.1.4 EPUB container fix
The former parser matched `<rootfiles>` when it was looking for `<rootfile>`. This caused the common error `Cannot resolve EPUB package` before the EPUB chapter data could load. The replacement checks XML start element names (also namespaced rootfile entries), skips comments, resolves the real OPF `full-path`, and includes host tests with generated standards-shaped EPUB files. This does not change USB Helper, saved book progress, or the application's Title ID.
