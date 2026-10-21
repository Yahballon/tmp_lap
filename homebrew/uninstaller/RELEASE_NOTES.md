# Uninstaller v0.1.7 — async startup scan and progress indicator

- Added animated blue scanning progress bar to the main screen.
- Initial inventory and Triangle refresh now run asynchronously; native PS5 renderer keeps updating instead of freezing during the scan.
- Added phase labels while scanning local folders and while reading the elfldr inventory.
- Input is locked during inventory replacement to avoid deleting stale entries.
- Existing confirmed removal dialogs and native/USB removal backends are retained, but console-side system uninstall completion has not yet been verified; do not test with an important game first.
- Logs continue at `/data/Uninstaller/uninstaller.log`.
