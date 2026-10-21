# Uninstaller v0.1.11 — unified game + ShadowMount USB source deletion

- Fix ShadowMount re-mounting games whose backing source persists in `/mnt/usbN/<TITLEID>-app`, including the reported `/mnt/usb0/PPSA21564-app`.
- One button: select a game and press X or Square, see one confirmation including the detected physical USB folder, press X to remove the registered title and matching USB source automatically.
- Uses the proven authenticated one-shot ELF helper with a read-only preflight mode. It only recognizes exact matching top-level `<TITLEID>-app` folders on USB 0–7 with game markers and consistent ShadowMount tracker; conflicting duplicate candidates are rejected.
- Move source to hidden same-volume quarantine before recursive verification and deletion so it won't be mounted again during cleanup. Logs leftover quarantine path on failure instead of claiming success. Refuses symlinks/special files and bounded traversal.
- Run deletion on a worker thread so the UI stays responsive; installed title cleanup and source deletion return separate outcome states.
- Keep existing native uninstall, USB PKG removal, USB homebrew, logos, scan progress and safe save-data behavior.
- Hardware results on firmware 5.10 must still be verified using a disposable USB-backed game.
