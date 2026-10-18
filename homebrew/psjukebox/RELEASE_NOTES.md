# PSJukebox v0.1.11 — Centered Covers and Live Navigation

- Vertically center the album coverflow (midpoint y=525); create a ~23.5px gap between the highlighted cover and adjacent jackets by increasing sleeve spacing to 380px
- L1/R1 switch artists consistently, including during song playback; the music keeps playing
- L2/R2 switch album covers consistently during playback; return to coverflow view when triggered from the playing/tracklist screen
- D-pad LEFT/RIGHT or either joystick LEFT/RIGHT: seek playing song -/+10 seconds without changing selected cover; analog tilt is edge-triggered to avoid rapid jumps
- D-pad DOWN or either joystick DOWN: open the selected album's Tracklist directly from carousel or while playback is displayed
- Preserve X Play/Tracklist, Square Pause/Resume, Triangle Favorite and OPTIONS Settings
- Auto-advance preserves the visible browsing context rather than snapping back to the playing album
- Tested with host layout/controller tests plus native PS5 build and package verification. Actual PS5 functionality still requires console validation

This release retains v0.1.10 packed .album, artist theming, Favorites, USB and SMB features.
