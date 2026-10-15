# PSTUBE v0.4.1 — Search Pagination Fix

- Fix partial final grid page: selecting the 19th or 20th video no longer leaves only two cards visible; the last six results always fill the page wherever possible.
- Native automatic YouTube continuation support. Scroll down to progressively load more results, 20 at a time; up to 400 real distinct videos per search.
- Real C++ HTTPS POST to YouTube search continuation endpoint. Use the version, visitor context and public API key supplied by YouTube, and remove duplicate IDs.
- Show results counter, page progress and loading status. Preserve DualSense navigation and playback.
- Keep GPU thumbnails bounded to prevent exhausting texture memory on long searches.
- Add regression tests for JSON continuation responses and live two-page retrieval in CI.

**Limitations:** YouTube may change the public search response or temporarily block automated paging; failures are shown instead of fake videos. The 400-result safety cap keeps memory bounded. Video playback availability depends on stream formats and YouTube access restrictions.

Install `PPSA99762.zip` from this release to `/data/homebrew/PPSA99762/`. Your saved favourites and download library are preserved.
