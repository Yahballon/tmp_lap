# Uninstaller v0.1.18 — Revert to the working v0.1.16 backend

User test: the v0.1.17 "stay open and press PS Home" workaround does not work reliably. Restore the proven v0.1.16 native GUI, deferred helper, one-time authentication and official ShadowMountPlus deletion logic. The only changes to these files are version/log labels.

- On a confirmed managed game deletion, Uninstaller again **exits automatically** after an authenticated helper acknowledgment. The independent ELF performs the physical source deletion only after ShadowMount permits it, then requests native PS5 title cleanup.
- Keep explicit confirmation of the exact physical source, bounded retries, protected ShadowMount API storage job validation, and persistent `/data/Uninstaller/deferred-shadow.log`.
- Remove the v0.1.17 "PS Home" overlay, journal polling while the app is running, and reliance on foreground focus switching.
- Keep USB PKG deletion, USB Homebrew removal and the validated classic two-pass uninstaller unchanged.
- Do not automatically touch stranded hidden quarantine folders from previous experimental versions.
