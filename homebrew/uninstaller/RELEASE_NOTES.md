# Uninstaller v0.1.8 — removal backend fix

The v0.1.7 PS5 log proves all removal attempts abort before dispatch because /data/Uninstaller/actions isn't private from the app sandbox.

- Replace blocked on-disk request folder with direct single-use authenticated localhost action broker. Upload via the same elfldr 9021 approach that made app discovery work; broker accepts one request via local port 9176 for up to 15 seconds.
- Wire title uninstall through native AppInstUtil instead of deleting registered game folders. Confirm result codes and distinguish asynchronous acceptance from actual completed removal.
- Permit opt-in USB PKG archive deletion, and one-folder-scoped USB homebrew removal with a bounded, prevalidated tree. Save data untouched.
- Add failure reasons and REMOVE_* diagnostic logs.
- No uncontrolled recursive deletion of installed games or protected paths.
- The PS5 runtime removal result is not yet hardware-verified; test disposable files first.
