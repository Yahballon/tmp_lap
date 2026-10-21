# Uninstaller v0.1.3 - find installed games and homebrew

- Fix false empty list: recognize LAPY/FAKE titles, -app/-app0 game folders, metadata-only entries, nested homebrew/games.
- Add USB0-USB7 and external drive ext0-ext3 scanning, data/etaHEN/games and alternate game roots.
- Log folder accessibility (root errors and errno), inspected children and per-root discovered entry counts under /data/Uninstaller/uninstaller.log.
- Probe app.db path read-only for future helper diagnosis; do not parse or modify system DB.
- System games and game dump folders remain non-removable; preserve game saves.
- This change requires console testing. Inaccessible roots cannot be listed without extra console permissions.
