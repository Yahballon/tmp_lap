# Ebook Reader v0.1.2

- Add an app-owned EPUB/PDF library at `/app0/assets/Books/`.
- Create the matching installation folder under `/data/homebrew/PPSA99741/assets/Books/`.
- Scan additional title-local `/download0/Books` and `/download0/ebooks` sources.
- Prefer writable per-app download storage when importing books.
- Retain saved reading progress and the original Title ID `PPSA99741`.

## Usage

The USB screenshot showed `Operation not permitted` for `/mnt/usb0`. That means direct browsing is restricted by the PS5 app sandbox. Copy EPUB files to the app's `assets/Books` directory using an already-authorized FTP/file manager, restart Ebook Reader, and press OPTIONS to rescan. Back up your book files before updating.

## Known limitations

The native app does not modify its filesystem privileges. Direct USB browsing remains unavailable if the loader denies it. PDF text extraction is limited. On-console testing is still required.
