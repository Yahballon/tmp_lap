# Ebook Reader v0.1.3 — USB EPUB Bridge

Fixes the situation where `/mnt/usb0/Books` is blocked by the native PS5 app sandbox.

- Bundles `bookscan.elf` and talks to an already-running `elfldr` at `127.0.0.1:9021` (same strategy as Payload LauncherX).
- The read-only helper scans `/mnt/usb0/Books`, including nested subfolders, and returns filenames and sizes to the native bookshelf.
- Books on the USB can be opened through a local file transfer on port `9175`, using the app's own temporary cache. No USB writes or sandbox changes.
- Preserves favourite books, reading progress and the original Title ID `PPSA99741`.

**Install:** Extract `PPSA99741.zip` into `/data/homebrew/` and ensure `PPSA99741/bookscan.elf` sits next to `eboot.bin`. Keep your existing book files and saved settings. Have the same elfldr service used by Payload LauncherX running on port 9021. Rescan using OPTIONS.

**Limitations:** Requires on-console firmware/loader testing; build success does not guarantee the helper can start on every firmware. EPUB support is text-focused and PDF support remains text preview only.
