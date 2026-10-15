# Ebook Reader v0.1.5 — EPUB Images & Cover Art

## Added
- Real EPUB2/EPUB3 covers embedded in the ZIP, displayed as proportional images on the native PS5 bookshelf.
- Inline JPEG/PNG/static GIF illustrations from EPUB chapters, in reading order, alongside existing page text.
- Bounded, lazy cover loading on a background worker; GPU textures are decoded and uploaded on the render thread.
- Reuse of previously downloaded USB book caches for faster cover loading.
- New EPUB regression fixtures with original JPEG front covers and PNG illustrations.

## Installation
Download `PPSA99741.zip` and replace the app folder inside `/data/homebrew/`, preserving your books and existing saved state. Title ID remains `PPSA99741`. Your existing USB helper (`bookscan.elf`, elfldr 9021 and bridge 9175) remains included and unchanged in behavior.

## Known limitations
- EPUB images are proportional, inline renderings in a simple reflow layout, not full XHTML/CSS layout. Unsupported image formats and external images are skipped.
- The PDF feature is still a text-only preview; a full native PDF page renderer will require a separate implementation.
- Real PS5 compatibility and performance must be verified on hardware.
