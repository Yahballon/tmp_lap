# Uninstaller v0.1.17 — Stay-open background ShadowMount deletion

Hardware confirmed that v0.1.16 works by shutting down the Uninstaller native PS5 title before a separate elfldr payload safely calls the ShadowMountPlus API. The user requested the app to remain open.

**New flow:** Confirm a managed ShadowMount game in the Uninstaller, then **press the PS button to go to PS Home without closing Uninstaller**. The PS5 runs Uninstaller in the background; the already authenticated, independent ELF helper continues the protected ShadowMount API deletion. The native app remains installed and running rather than calling the system exit function automatically. Return to Uninstaller to view the latest result.

- Preserve the proven v0.1.16 authenticated one-shot helper, physical source identity validation, bounded busy retries, official delete job status checks, original native title cleanup and append-only journal.
- Keep foreground UI active after helper acknowledgment and show a clear "BACKGROUND UNINSTALL" status screen instructing the player to navigate to PS Home (not close the app).
- When the native UI runs, it periodically reads `/data/Uninstaller/deferred-shadow.log` for `DEFER_START`, `DEFER_WAIT`, `DEFER_QUEUED`, `DEFER_PROGRESS`, `DEFER_SOURCE_REMOVED`, `DEFER_SOURCE_DONE_TITLE_RECHECK`, `DEFER_FAIL` and `DEFER_UNCERTAIN`. Show real job percentage when supplied; rescan the installed-title list after logged successful source cleanup.
- While the deferred operation is underway, new destructive input is blocked; Circle remains a manual fallback to exit the native UI, letting the helper continue like v0.1.16.
- ShadowMountPlus source code allows background PPSA games after a confirmed AppFocus change; **firmware 5.10 behavior still requires hardware validation**. Deletion may remain blocked if going to PS Home does not clear focus. Do not bypass ShadowMount protection or claim a completed deletion from helper acknowledgment.
- No changes to the existing USB PKG, USB Homebrew or classic two-pass removal routines; savegames remain untouched.

Important: the PS5 Home screen and control center may have different focus semantics; prefer fully switching to the Home screen instead of only opening the quick control center. Do not open another game during deletion. If busy persists, press Circle in Uninstaller to close the app; the helper can then finish through the v0.1.16 behavior.
