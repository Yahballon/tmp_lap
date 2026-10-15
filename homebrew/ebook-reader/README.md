# Ebook Reader v0.2.3 — USB bridge support

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

Release tag: `ebook-reader-v0.2.3`.

Source license: GPL-3.0-or-later, respecting dependency licenses.

## v0.1.6 EPUB container fix
The former parser matched `<rootfiles>` when it was looking for `<rootfile>`. This caused the common error `Cannot resolve EPUB package` before the EPUB chapter data could load. The replacement checks XML start element names (also namespaced rootfile entries), skips comments, resolves the real OPF `full-path`, and includes host tests with generated standards-shaped EPUB files. This does not change USB Helper, saved book progress, or the application's Title ID.

## v0.1.6: Real EPUB cover art and inline illustrations

- **Real library covers:** EPUB2 `<meta name="cover" content="…">`, EPUB3 `properties="cover-image"`, and sane cover-file/first-image fallbacks. Cover is decoded from the ZIP file and displayed as a GPU texture in the wooden bookshelf.
- **Illustrations in the reader:** Embedded JPEG, PNG, and static GIF assets referenced by `<img src="…">` or `<image href="…">` are resolved relative to their XHTML chapter, extracted from the archive, and shown inline in the correct text order. Images are proportionally fitted inside the two-column reading pages.
- **Fast UI:** Cover extraction and optional USB transfer run on a background pthread. OpenGL texture creation stays on the rendering thread; the texture cache and individual EPUB image sizes are bounded. Previously transferred USB books are reused when possible.
- **No external assets:** HTTP images and scripts are never fetched or executed. Compressed ZIP entries, decoded artwork and image count are limited to protect memory.
- **Tests:** Generated EPUB2/3 packages with actual original JPEG covers and PNG illustrations are verified in CI.

**Limitations:** This is not a CSS/XHTML layout engine. Relative inline images use reserved blocks in the book's existing paginated text layout. Animated GIF, SVG, unsupported formats, externally hosted images and DRM are unsupported. **PDF mode remains extractable-text preview**; displaying real PDF pages and their image objects needs an independently ported PDF renderer (not included in v0.1.6). Console-side verification required.

Release: https://github.com/Yahballon/tmp_lap/releases/tag/ebook-reader-v0.1.6

## v0.1.6: CBR / CBZ comics and in-reader zoom

- **CBR** (RAR4/RAR5) and **CBZ** (ZIP) image comic files are now recognized in the bookshelf and by the existing read-only USB helper.
- Comic images are sorted in natural numeric order (e.g. page2 before page10) and decoded one page at a time using the cross-compiled PacBrew **libarchive**.
- First comic page becomes the library cover. Images are fitted to the TV area while maintaining aspect ratio; page textures are decoded and downscaled to at most 2048px on the longest side.
- **Zoom:** L2 = zoom out, R2 = zoom in (100–400% in 25% increments), L3/OPTIONS = fit/reset; D-Pad = pan while zoomed; L1/R1 = previous/next page; X = next; Circle = bookshelf. Zoom also works in EPUB/PDF text preview.
- Cover extraction runs in a worker and comic pages are decoded one at a time on a worker; rendering and OpenGL uploads stay on the main thread.
- Existing EPUB cover/inline imagery, PDF text preview, bookmarks, USB bridge and Title ID `PPSA99741` are preserved.

**Limits:** Supports unencrypted single-file RAR4/RAR5, ZIP, JPEG/PNG/GIF (not WEBP by stb decoder in this build); no password-protected or multipart archives. Comic pages: up to 20 MiB uncompressed image data and 2048px displayed texture size. Archive maximum 512 MiB via USB bridge, 1,800 pages indexed. Unreadable files show errors; some RAR compression features may be unsupported by libarchive on hardware. This does **not** add graphical PDF page rendering. Tests must pass in CI, and PS5 hardware testing remains outstanding.

## v0.2.3: Bookshelf layout and zoom reset

- Two non-overlapping rows with five books each per screen, so real books no longer collide with covers on the next shelf. Multi-page collections remain navigable.
- Real JPEG/PNG/CBR/CBZ covers occupy the entire book front using centre-cropped UVs rather than a small aspect-fit thumbnail, and selection is kept around the frame.
- Every page change keeps the zoom factor but resets panning to start at the top edge. Comic pages are top-aligned even when the original picture is letterboxed; EPUB/PDF text starts from the first text row. D-pad panning is still available after the page change.
- A geometry regression test covers shelf spacing, crop ratios and top alignment for 125–400% zoom.

Release: https://github.com/Yahballon/tmp_lap/releases/tag/ebook-reader-v0.2.3

## v0.2.3: Reach the full bottom of zoomed pages

Zoom panning boundaries are now calculated from the actual rendered image rectangle (portrait, landscape, and square) instead of a fixed approximation. At 125%–400%, the D-Pad can reach the final image row. Opening a new page preserves the chosen zoom but resets scrolling to its beginning.

**Square** toggles **Single Page / Double Page** for EPUB and PDF text-reading mode. Double Page displays two consecutive text columns (34 lines); Single Page displays a single centred paper page (17 lines). The reading position is preserved when switching. L1/R1 and X continue turning pages; progress remains saved. Comics retain their single full-screen panel and existing Square = Last Page shortcut.

Release: https://github.com/Yahballon/tmp_lap/releases/tag/ebook-reader-v0.2.3

## v0.2.3: Fix Single Page text overflow at zoom

Single-page EPUB/PDF text and illustrations are now clipped to the physical sheet instead of the full spread; at 175% and other zoom levels the first glyph lines up with the paper's left margin. Pan can still reach the far right and bottom. The page number, reading mode and zoom percentage are shown beneath the book, not on top of the text. USB/comics/saved progress remain unchanged.

Release: https://github.com/Yahballon/tmp_lap/releases/tag/ebook-reader-v0.2.3

## v0.2.3 — Full-width single-page layout

Square switches EPUB/PDF to a full-width parchment page, using the same book box as the double-page reader but displaying one reflowed column across the entire reading canvas. There are no black/brown side gutters in Single Page mode. Text is intelligently wrapped to around 89 characters per line (instead of retaining old narrow 49-character lines); inline illustrations are remapped, and switching views preserves the approximate source-text position. The existing 100–400% zoom/pan, USB support, saved progress and CBR/CBZ remain working.

Release: https://github.com/Yahballon/tmp_lap/releases/tag/ebook-reader-v0.2.3

## v0.2.3 — Better CBR/RAR support

- CBR files are **RAR4 or RAR5 comics containing JPEG/PNG/GIF image pages**, not a standalone image format. The native comic reader can decode these from USB or local libraries. The first page is used as cover artwork and pages are sorted numerically.
- Fix local `.cbr` and `.cbz` automatically appearing under **Comics**, even without “comic” in the filename.
- Improved acceptance of genuine RAR entries with Windows-style `\\` path separators, optional `./` prefixes or unspecified file type/size metadata, while rejecting traversal paths and links.
- Validate each decompressed picture's JPEG/PNG/GIF signature and show a useful error for corrupted, encrypted or unsupported entries.
- GitHub Actions now tests actual compressed RAR4 **and** RAR5 member bytes, not merely a header, alongside CBZ images and USB scanning.

Usage: place `.cbr` in `/mnt/usb0/Books/` (or the app-specific local Books directory), refresh via OPTIONS, choose **Comics** with L1/R1, then X to open. Keep the existing `elfldr` USB helper running on port 9021. Password-protected or multi-volume RAR files cannot be read.

## v0.2.2: Fix large USB CBR transfers

- Larger comics (up to 2 GiB) can be catalogued and copied, instead of the previous 512 MiB limit.
- Dedicated book helper remains available for 10 minutes of idle browsing rather than stopping after 15 seconds; transfer and socket timeouts are increased for slow USB reads/writes.
- **Resumable transfer:** the reader now requests the remaining bytes if the USB helper disconnects halfway through a file, rather than discarding the transferred part. Three recovery attempts are allowed per attempt.
- Book cache selection checks writable storage and enough free space before beginning a large transfer. Errors identify whether the USB socket, cache write or storage capacity was the problem, including transferred MiB.
- Foreground and background USB copy operations are serialized; large USB comics are not copied merely to retrieve bookshelf cover thumbnails.
- The bridge listens on local port **9176** for this new version, ensuring that the packaged updated helper is launched instead of a stale v0.2.1 helper on 9175. A compatible elfldr must still run on **9021**.
- Includes a host integration test of a 16 MiB simulated CBR, forced mid-stream disconnection, correct byte-for-byte resume, and concurrent requests. PDF/EPUB/CBZ and all saved progress remain compatible.

Install the new PPSA99741.zip from TESTBUILDS without deleting saved data. USB books remain at `/mnt/usb0/Books`. A full comic must fit in available writable PS5 cache space to open; this implementation does not stream directly into the RAR decoder. Physical PS5 testing is still required.

## v0.2.3 — PS5 SIGSEGV crash / missing cover fix & local diagnostics

- Fixed a cover-worker deadlock introduced in v0.2.2: encountering a remote CBR/CBZ returned without clearing the busy flag, preventing *all* later EPUB covers from loading. Skipped remote comic covers no longer block other thumbnails. Local EPUB/CBR/CBZ covers remain supported.
- Removed the unverified `statvfs` call from the **PS5 USB-book opening** code, which was newly added in v0.2.2 and could trigger a runtime fault on the console. Cache writability is now detected using verified mkdir/fopen/fputc/fflush operations, while disk-full errors are reported on actual writes.
- Added persistent diagnostics: `/data/EbookReader/ebook-reader.log`. The app logs startup, library scans, cover results, book-open attempts, USB copy progress, resumes and errors. The log rotates at 2 MiB (previous copy `.log.1`). If the directory is not writable, the app continues and shows a warning.
- Added local book folder `/data/EbookReader/Books`. Place EPUB, PDF, CBR or CBZ there (including nested folders). It is scanned into My Library alongside the prior paths. The import list can also see this folder, and book imports prefer it when writable.
- Retains cached transfers, RAR4/RAR5, CBZ, EPUB images, full-width reading, zoom, DualSense controls and existing saved progress.
- A host regression test verifies all four file types and logging in a simulated data folder. PS5 workflow checks that unverified statvfs is absent and that skipping a remote comic cannot block later cover jobs.

**Install:** Update to `PPSA99741.zip` with TESTBUILDS. Do not delete saved data. USB still uses `elfldr` 9021 and book helper 9176.

**Debugging:** After reproducing a problem, get `/data/EbookReader/ebook-reader.log` with your PS5 file manager and share its last lines. Log creation depends on the PS5 sandbox allowing writes to this path. Actual console confirmation is required.

Release: https://github.com/Yahballon/tmp_lap/releases/tag/ebook-reader-v0.2.3

## v0.2.4: Fix ENOSPC on USB CBR/CBZ books (cache quota)

PS5 runtime logs revealed USB transfer was working, but `/download0/ebook-reader-cache` repeatedly failed around **21 MiB** with **No space left on device**. The entire RAR/CBR archive must be copied before libarchive can open it.

- **Preferred large-comic cache:** `/data/EbookReader/Cache/`. The previously small-quota `/download0` location is a last-resort fallback, not the default. Existing verified files in older caches remain readable without downloading them again.
- **Automatic cleanup:** The reader keeps up to approximately **768 MiB** of disposable USB book copies, evicting oldest copies first. It also removes abandoned `.part` files from interrupted transfers. It never deletes anything from `/data/EbookReader/Books/`, user settings, library progress, or logs.
- A cache cleanup is attempted **before** a new download; size/magic validation is retained for cached EPUB, PDF, CBR and CBZ.
- Retains resumable transfers and detailed USB diagnostic events. An explicit local file under `/data/EbookReader/Books/` can also be opened with no USB transfer or cache copy.
- Host CI verifies a **48 MiB** simulated CBR transfer to the new data-root cache and tests that cleanup preserves books and settings.
- If `/data/EbookReader/Cache` also lacks space or write privileges, the reader still reports the storage failure. The file cannot be read until a suitable writable filesystem has enough room.

**Install:** Update `PPSA99741.zip` via TESTBUILDS, keeping saved data. USB books remain under `/mnt/usb0/Books`. Diagnostic log: `/data/EbookReader/ebook-reader.log`.

Release: https://github.com/Yahballon/tmp_lap/releases/tag/ebook-reader-v0.2.4
