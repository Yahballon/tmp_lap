# Ebook Reader v0.2.2 — Large USB CBR transfer fix

## Fixed
- Removed the fragile five-second transfer timeout; the client now waits up to 60 seconds and the USB helper up to 90 seconds per socket operation.
- Read-only helper remains available for 10 minutes of idle browsing, supports comic archives up to 2 GiB and exposes a fresh bridge endpoint on localhost port 9176.
- If a comic transfer is interrupted, the reader tries to resume from the last successfully written byte (up to three retries).
- Parallel background/foreground USB fetches can no longer compete for the single-threaded helper. The bookshelf will not pre-download huge USB CBR/CBZ files just to obtain cover thumbnails.
- Cache location is selected based on available free space before copying. A failed download now reports its stage and MiB transferred instead of the vague 'USB transfer incomplete or cache write failed'.

## Verification
- Native ELF/helper/ZIP compilation and archive tests.
- New host-side integration test sends a synthetic 16 MiB CBR through the helper with a deliberately interrupted connection and checks the exact restored bytes and concurrent request serialization.

## Install
Update to `PPSA99741.zip` through TESTBUILDS. Leave `elfldr` active on port 9021 and your comics under `/mnt/usb0/Books`. The new app uses book bridge port 9176 and preserves existing books, settings, progress, CBR/CBZ and EPUB/PDF support.

## Known limitations
The console still requires enough writable storage to cache the complete CBR file. Password-protected and multipart RAR files are unsupported; actual PS5 validation is pending.
