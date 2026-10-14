# Ebook Reader v0.1.1

- Fix missing EPUB/PDF files on USB folders, including `/mnt/usb0/Books` and nested author directories.
- Expand USB mount scanning, extension matching and readable-file fallbacks for unreliable `stat` results.
- Add USB PATH STATUS diagnostics in the import view, and expose counts after rescan.
- Keep existing local favorites and reading progress across the update.

**Install:** Extract `PPSA99741.zip` into `/data/homebrew/`, replace the previous application files without deleting your reading data, then refresh the homebrew launcher.

**Known limitations:** This is a native EPUB text reader; PDF remains a best-effort text preview. PS5 USB mount accessibility depends on your jailbreak/loader; diagnostics now show mount errors instead of silently appearing empty.
