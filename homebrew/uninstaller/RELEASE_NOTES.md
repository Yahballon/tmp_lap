# Uninstaller v0.1.2 — crash instrumentation

- Persist early-start diagnostic log at `/data/Uninstaller/uninstaller.log`.
- Add best-effort native signal crash report at `/data/Uninstaller/crash.log`.
- Render the first frame before filesystem scans and log the last scanned root.
- Avoid unverified PS5 `statvfs` calls suspected in the startup crash; capacity is shown as unavailable.
- Keep PPSA99986 and preserve game save data. Installed game uninstall remains disabled until verified.

Crash fix is not claimed hardware-verified until tested on a jailbroken console.
