# Uninstaller v0.1.15 — ShadowMount-first removal

- The same X/Square confirmation now prefers the official ShadowMountPlus 1.7 deletion API for verified managed game folders and backing images.
- Reads and confirms the physical source path and management status before issuing `/api/v1/games/delete`.
- Tracks the asynchronous job through `/api/v1/games/storage/status`; validates job ID, operation, title ID and source path on every response, and displays actual progress.
- On confirmed source deletion, asks PS5 AppInstUtil to clean up the remaining title registration, followed by inventory rescan.
- Preserves the v0.1.14 classic validated `tree_walk`/unlink/rmdir fallback for titles not managed by ShadowMount or if its compatible API is unavailable.
- Refuses unsafe bypass when ShadowMount recognizes a source but denies access; no raw source delete after API failure or uncertain job status.
- No changes to USB PKG, homebrew or save-data behavior. Hardware verification of the new ShadowMount route is still required.
