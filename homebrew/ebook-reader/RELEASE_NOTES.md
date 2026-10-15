# Ebook Reader v0.1.4 — EPUB package parsing fix

### Fixed

- Standard EPUB archives with `META-INF/container.xml` were incorrectly rejected with **Cannot resolve EPUB package**. The parser matched the plural `<rootfiles>` wrapper instead of the actual `<rootfile full-path="...">` element.
- Match complete XML element names instead of string prefixes, including namespaced `<ocf:rootfile>` and `<item>`/`<itemref>` package elements.
- Handle single/double quoted and whitespace-separated XML attributes, comments containing decoy tags, and multiple rootfile declarations.
- Preserve bounded archive decoding, path traversal protections, and existing local/USB book handling.

### Testing

Added generated EPUB fixtures for standard and namespaced package containers, comments, multiple rootfiles, and invalid paths. CI must pass EPUB extraction tests and full native PS5 ZIP validation before release.

### Install

Update to `PPSA99741.zip` from this release through TESTBUILD APP. Same Title ID `PPSA99741`, and same `bookscan.elf` USB bridge and ports `9021/9175`.

### Known limitations

EPUB text reflow only; image-heavy layouts, DRM and advanced CSS are unsupported. PDF remains a text preview. Actual PS5 reading of your books still requires a console test.
