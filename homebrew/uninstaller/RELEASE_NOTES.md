# Uninstaller v0.1.14 — Game/source diagnostics and reliable classic removal

PS5 v0.1.13 console log shows successful AppInstUtil uninstall-request return codes for multiple games, but `SHADOWMOUNT_SOURCE none` during preflight, so no verified USB game data was deleted. Some registrations appeared absent in subsequent scanner outputs; completion/re-mount status must be verified.

- Keep the classic tree_walk + unlink + rmdir deletion backend of v0.1.11 intact.
- Extend conservative ShadowMount source discovery to exact `<TITLEID>-app0` as well as `<TITLEID>-app` on USB0–USB7, without weakening mount-tracker checks or traversing unrelated folders.
- Add read-only `SOURCE_DIAG` logs for mount.lnk, mount_img.lnk, existing rejected source directories and exact reason they were rejected.
- Stream helper output line-by-line without the previous 4 KiB total-output cutoff that could prematurely abort large classic game deletions; preserve final authenticated results, selected source and quarantined path on failure.
- Show/log `REMOVE_VERIFY_PRESENT` / `REMOVE_VERIFY_ABSENT` after the automatic inventory rescan. Distinguish an accepted Sony uninstall request from confirmed disappearance in the inventory.
- Retain one-button X/Square confirmation, PS5 AppInstUtil, USB Homebrew and PKG deletion, no save-data deletion.
- No runtime removal result is claimed for v0.1.14 prior to PS5 hardware testing.
