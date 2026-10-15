# Ebook Reader v0.2.4 — Fix CBR / CBZ USB cache full

## Root cause
PS5 diagnostics from v0.2.3 prove that the USB helper and RAR readers work, but writing a new comic to `/download0/ebook-reader-cache` stopped around 21 MiB with `ENOSPC`. This was a filesystem cache problem, not an unsupported comic format.

## Fixed
- Save new downloaded USB books into writable `/data/EbookReader/Cache/` first instead of the smaller `/download0` app cache.
- Reuse valid cached files already present under the old paths without fetching them again.
- Automatically evict the oldest disposable USB-cache files when the 768-MiB soft budget would be exceeded, including abandoned incomplete `.part` copies.
- Preserve user-owned files and folders: `/data/EbookReader/Books/`, saved settings/progress and `ebook-reader.log` are never removed by cache cleanup.
- Keep resumable transfers, native RAR4/RAR5 decoder, EPUB/PDF and single/double-page zoom.
- CI adds a 48-MiB CBR transfer test verifying the data-root cache path, plus cache-cleanup and user-book-protection assertions.

## Install and verify
Install `PPSA99741.zip` through TESTBUILDS. Do not delete books or saved data. With `elfldr` on port 9021 and the book helper on 9176, choose a CBR on USB. Check `/data/EbookReader/ebook-reader.log` for `USB_FETCH_OK /data/EbookReader/Cache/...`, then `OPEN_OK`.

## Limits
Copying a full comic into the cache still requires actual free space and write access under `/data/EbookReader`; this is not an on-demand RAR streaming reader. Current tests cover native compilation, transfer and safe eviction, while console hardware confirmation remains pending.
