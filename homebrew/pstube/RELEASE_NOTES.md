# PSTUBE v0.5.1 — Search List and Navigation Fix

- Fix the reported issue: at the last result (e.g., 19/20), PSTUBE previously displayed only the final two videos. It now fills the final TV grid with the preceding results, retaining selection.
- Display the visible result range, the total number of fetched videos, loading-more status and a scroll progress bar.
- Preserve automatic YouTube search continuation from v0.5.0 to load additional video pages on D-Pad/left-stick navigation.
- Add a host-side native C++ two-page live YouTube search test to CI.
- Preserve the existing native player, stream playback, downloads and user settings. No PC or companion tools.

**Limitations:** Public YouTube APIs can change, rate-limit or block searches. The total result count indicates loaded videos, not a promise of all results on YouTube. The app remains subject to PS5 hardware testing.

Install: replace /data/homebrew/PPSA99762 from the verified release zip and update TESTBUILDS. Saved preferences, favourites and downloads remain untouched.
