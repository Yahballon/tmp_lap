# Uninstaller v0.1.6 – actual controlled uninstall experiments

- Remove permanent Found X entries overlay.
- Reject false tiny drive capacities, e.g. /mnt/ext0=2 MB, and never default to a fake mount.
- Enable explicit native AppInstUtil uninstall dispatch for installed titles where title ID and /user/app/<titleId> match. Two-step confirmation and private one-shot payload with request nonce. Native API may be asynchronous; acceptance must not be confused with completed removal.
- Allow deleting explicit USB .pkg files via a privileged helper with filesystem/path checks. Installed games are not removed when clearing a USB archive.
- Custom mount game-dump folders and privileged homebrew folders remain read-only (not safely uninstallable by deleting arbitrary directories).
- Protect self PPSA99986 and keep saves untouched; report all actions under /data/Uninstaller.
- Firmware 5.10 hardware support requires testing: start with a disposable homebrew app, not a valuable retail game.
