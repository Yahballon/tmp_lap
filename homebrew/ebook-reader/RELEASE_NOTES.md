# Ebook Reader v0.2.6 — Exit Confirmation

## New
- Circle on the My Library home screen opens a Yes/No confirmation dialog instead of closing the app immediately.
- Cross selects **Yes** and exits; Circle selects **No** and returns to My Library.
- Modal consumes input so it cannot accidentally open another book or navigate under the confirmation.
- Existing Circle / Back controls in the book reader and USB Storage screen are unchanged.

## Safe shutdown
- Save existing book reading state before quitting.
- Wait for any detached thumbnail, image or book-load workers to finish before releasing PS5 rendering resources.
- Leave all local books, existing covers, settings and USB cache untouched.

## Verification
- Host controller-state tests cover confirmation, cancellation, accidental simultaneous presses and repeated opens.
- GitHub Actions compiles the native PS5 application and validates a complete PPSA99741.zip. Real PS5 hardware testing still needed.

## Installation
Install PPSA99741.zip through TESTBUILDS and keep your existing /data/EbookReader/Books and settings intact. Title ID PPSA99741.
