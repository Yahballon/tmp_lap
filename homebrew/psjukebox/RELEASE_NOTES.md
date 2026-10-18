# PSJukebox v0.1.22 — Flicker-free Coverflow

- Keep selected and neighboring jackets resident in GPU cache, preventing mid-navigation eviction and cover reloading.
- Request destination and nearby jackets before optional logos and album backgrounds.
- Wait for a pending cover to finish loading before beginning a 480 ms carousel transition, preserving the current image in the meantime.
- Truly missing cover artwork still uses the normal text fallback.
- Retain SMB/USB support, favorites, playback, DualSense navigation, and all v0.1.21 UI improvements.

GitHub Actions verifies compilation and the installable ZIP. Confirm results on PS5 hardware.
