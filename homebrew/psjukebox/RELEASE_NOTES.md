# PSJukebox v0.1.13 — Header Credits and Build Version

- Display **by YAHBALLON · v0.1.13** just below the time at the top right of all PSJukebox screens
- Right-align the subtle text with the existing clock, without changing the center tabs or theme
- Read the displayed version from the native build version constant rather than hardcoding it separately in the label
- Add CI check matching the on-screen version to app.json release metadata
- Keep all prior v0.1.12 UI, gamepad controls, packed .album support, SMB, favorites and playback behavior

PS5-build and automated host tests validate the change; final visual verification requires a console test.
