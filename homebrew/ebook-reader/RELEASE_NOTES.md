# Ebook Reader v0.2.3 — Fix book-opening SIGSEGV and missing covers

## Fixed
- The v0.2.2 remote CBR/CBZ cover skip permanently set `g_coverBusy`, preventing cover extraction for unrelated EPUB books. This is corrected.
- Removed the recently introduced, unverified POSIX `statvfs` invocation from the PS5 USB cache-selection path, replacing it with mkdir/fopen/write probes. Transfers report write/flush errors directly.
- Background cover jobs no longer block the rest of the bookshelf when a remote comic has no readily available art.

## Added
- A dedicated PS5 book root: `/data/EbookReader/Books`, discovered recursively together with all previous library paths (EPUB, PDF, CBR, CBZ).
- Persistent diagnostic log: `/data/EbookReader/ebook-reader.log`, including startup, book-open events, USB transfers/retries, and errors. Rotates at 2 MiB.
- CI regression tests for the local folder and log, plus source guards against the crash regression and cover-worker starvation.

## Installation
Install `PPSA99741.zip` from TESTBUILDS, keeping Title ID `PPSA99741` and saved library state. Do not remove existing USB books; `elfldr` 9021 and the USB book helper on 9176 remain supported.

## Known limitations
The identified crash triggers have been removed but actual PS5 testing is still required to confirm they explain the SIGSEGV screenshot. If another crash occurs, inspect `/data/EbookReader/ebook-reader.log` for the last successful operation. Logs are best-effort if file permissions restrict access. PDF remains a text preview; password-protected/multipart RAR remains unsupported.
