# Uninstaller v0.1.13 — Classic Delete restored

The Turbo Delete approach introduced in v0.1.12 did not work on the user's PS5. This release restores the fully working deletion backend of v0.1.11, without changing the UI workflow.

- Restore the original two-pass USB game deletion: `tree_walk` first validates entries, then safely unlinks regular files and removes empty directories.
- Retain the immediate same-volume quarantine rename of the exact `/mnt/usbN/<TITLEID>-app` source to prevent ShadowMount from re-mounting the game during removal.
- Restore the prior helper's working one-shot response protocol; remove Turbo Delete progress handling and optimized `openat`/`unlinkat` code.
- Retain the unified X/Square confirmation, normal PS5 AppInstUtil uninstall, USB PKG and USB homebrew delete, scanning and title safety checks.
- Existing hidden `.uninstaller-...` leftovers from a failed 0.1.12 delete are **not** removed automatically.
